// server.cpp
// ConcurMeet - Room-enabled chat server (Linux/POSIX)
// Features:
//  - Commands: /create <room>, /join <room>, /leave, /listrooms, /users
//  - Per-room chat and per-room logs (room_<room>.txt)
//  - Global chat log (chatlog.txt)
//  - Thread-per-client with mutex-protected shared state
// Build:
//   g++ -std=c++17 server.cpp -o server -pthread
// Run:
//   ./server

#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <algorithm>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <signal.h>

using namespace std;

// Global shared server state
vector<int> clients;                 // all connected client sockets
map<int, string> usernames;          // socket -> username
map<int, string> clientRoom;         // socket -> current room (empty if none)
map<string, set<int>> rooms;         // room_id -> set of client sockets in room

mutex stateMutex;                    // protects all above maps/vectors

ofstream globalLog("chatlog.txt", ios::app);
mutex globalLogMutex;

// Utility: safe write to global log
void writeGlobalLog(const string &s) {
    lock_guard<mutex> lock(globalLogMutex);
    if (globalLog.is_open()) {
        globalLog << s << endl;
        globalLog.flush();
    }
}

// Utility: write per-room log (open/append each time for simplicity)
void writeRoomLog(const string &room, const string &s) {
    string fname = "room_" + room + ".txt";
    lock_guard<mutex> lock(stateMutex); // protect concurrent log file writes that could race with room membership changes
    ofstream fout(fname, ios::app);
    if (fout.is_open()) {
        fout << s << endl;
        fout.close();
    }
}

// Send text to a specific client socket (adds newline if not present)
void sendToClient(int clientSock, const string &msg) {
    string out = msg;
    if (!out.empty() && out.back() != '\n') out += "\n";
    send(clientSock, out.c_str(), (int)out.size(), 0);
}

// Broadcast to all clients in a room (optionally excluding sender)
void broadcastToRoom(const string &room, const string &msg, int excludeSock = -1) {
    string out = msg;
    if (!out.empty() && out.back() != '\n') out += "\n";

    // collect recipients under lock
    vector<int> recipients;
    {
        lock_guard<mutex> lock(stateMutex);
        auto it = rooms.find(room);
        if (it != rooms.end()) {
            for (int s : it->second) {
                if (s != excludeSock) recipients.push_back(s);
            }
        }
    }

    // send without holding the lock
    for (int r : recipients) {
        send(r, out.c_str(), (int)out.size(), 0);
    }

    // also write to room log & global log
    writeRoomLog(room, msg);
    writeGlobalLog("[room:" + room + "] " + msg);
}

// Helper: return number of users in a room
int roomCount(const string &room) {
    lock_guard<mutex> lock(stateMutex);
    auto it = rooms.find(room);
    if (it == rooms.end()) return 0;
    return (int)it->second.size();
}

// Helper: send list of rooms to a client
void sendRoomList(int clientSock) {
    lock_guard<mutex> lock(stateMutex);
    if (rooms.empty()) {
        sendToClient(clientSock, "No rooms exist. Create one with: /create <room>");
        return;
    }
    ostringstream oss;
    oss << "Rooms (" << rooms.size() << "):";
    for (auto &p : rooms) {
        oss << "\n- " << p.first << " (" << p.second.size() << " users)";
    }
    sendToClient(clientSock, oss.str());
}

// Helper: send users in current room to client
void sendUsersInRoom(int clientSock, const string &room) {
    lock_guard<mutex> lock(stateMutex);
    auto it = rooms.find(room);
    if (it == rooms.end() || it->second.empty()) {
        sendToClient(clientSock, "Room is empty or does not exist.");
        return;
    }
    ostringstream oss;
    oss << "Online users in " << room << " (" << it->second.size() << "):";
    for (int s : it->second) {
        auto itName = usernames.find(s);
        if (itName != usernames.end()) oss << "\n- " << itName->second;
    }
    sendToClient(clientSock, oss.str());
}

// Add client to a room (creates room if createIfMissing==true)
bool addClientToRoom(int clientSock, const string &room, bool createIfMissing = true) {
    lock_guard<mutex> lock(stateMutex);
    if (!createIfMissing && rooms.find(room) == rooms.end()) return false;
    rooms[room].insert(clientSock);
    clientRoom[clientSock] = room;
    return true;
}

// Remove client from their current room (if any)
string removeClientFromRoom(int clientSock) {
    lock_guard<mutex> lock(stateMutex);
    string oldroom;
    auto it = clientRoom.find(clientSock);
    if (it != clientRoom.end()) {
        oldroom = it->second;
        auto rit = rooms.find(oldroom);
        if (rit != rooms.end()) {
            rit->second.erase(clientSock);
            // if room becomes empty we keep it (optional: erase empty rooms)
            // if you want to auto-delete empty rooms uncomment:
            // if (rit->second.empty()) rooms.erase(rit);
        }
        clientRoom.erase(it);
    }
    return oldroom;
}

// Cleanly remove client from global lists (on disconnect)
void cleanupClient(int clientSock) {
    string username;
    string roomLeft;

    {
        lock_guard<mutex> lock(stateMutex);
        auto itName = usernames.find(clientSock);
        if (itName != usernames.end()) username = itName->second;
    }

    roomLeft = removeClientFromRoom(clientSock);

    {
        lock_guard<mutex> lock(stateMutex);
        clients.erase(remove(clients.begin(), clients.end(), clientSock), clients.end());
        usernames.erase(clientSock);
    }

    if (!username.empty()) {
        if (!roomLeft.empty()) {
            string msg = username + " left the chat (" + to_string(roomCount(roomLeft)) + " online)";
            broadcastToRoom(roomLeft, msg, -1);
        }
        writeGlobalLog("SERVER: " + username + " disconnected");
        cerr << "Client disconnected: " << username << " (sock=" << clientSock << ")\n";
    } else {
        cerr << "Client disconnected (sock=" << clientSock << ")\n";
    }

    close(clientSock);
}

// Trim helper
static inline string trim(const string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

// Per-client thread
void handleClient(int clientSock) {
    char buffer[2048];

    // 1) Receive username first
    int r = recv(clientSock, buffer, sizeof(buffer)-1, 0);
    if (r <= 0) {
        close(clientSock);
        return;
    }
    buffer[r] = '\0';
    string username = trim(string(buffer));

    {
        lock_guard<mutex> lock(stateMutex);
        usernames[clientSock] = username;
    }

    // add to global clients list
    {
        lock_guard<mutex> lock(stateMutex);
        clients.push_back(clientSock);
    }

    cerr << "New connection: " << username << " (sock=" << clientSock << ")\n";
    writeGlobalLog("SERVER: " + username + " connected");

    // No automatic room join — user must /create or /join
    sendToClient(clientSock, "Welcome " + username + "! Create or join a room:");
    sendToClient(clientSock, "Commands: /create <room> | /join <room> | /leave | /listrooms | /users");

    // message loop
    while (true) {
        int bytes = recv(clientSock, buffer, sizeof(buffer)-1, 0);
        if (bytes <= 0) {
            // disconnect
            cleanupClient(clientSock);
            return;
        }
        buffer[bytes] = '\0';
        string raw = trim(string(buffer));
        if (raw.empty()) continue;

        // If starts with '/', treat as command
        if (raw[0] == '/') {
            // tokenize
            istringstream iss(raw);
            string cmd;
            iss >> cmd;

            if (cmd == "/create") {
                string room;
                iss >> room;
                if (room.empty()) {
                    sendToClient(clientSock, "Usage: /create <room>");
                    continue;
                }

                {
                    lock_guard<mutex> lock(stateMutex);
                    if (rooms.find(room) == rooms.end()) {
                        rooms[room] = set<int>();
                    }
                }
                // move client into room
                string prev = removeClientFromRoom(clientSock);
                addClientToRoom(clientSock, room, true);

                string usernameLocal;
                {
                    lock_guard<mutex> lock(stateMutex);
                    usernameLocal = usernames[clientSock];
                }

                // announce in room
                string joinMsg = usernameLocal + " joined the chat (" + to_string(roomCount(room)) + " online)";
                broadcastToRoom(room, joinMsg, clientSock); // exclude sender from immediate echo
                sendToClient(clientSock, "You have created and joined room: " + room);
                writeGlobalLog("SERVER: " + usernameLocal + " created room " + room);
                writeRoomLog(room, "SYSTEM: " + usernameLocal + " created and joined room");
                continue;
            }
            else if (cmd == "/join") {
                string room;
                iss >> room;
                if (room.empty()) {
                    sendToClient(clientSock, "Usage: /join <room>");
                    continue;
                }
                {
                    lock_guard<mutex> lock(stateMutex);
                    if (rooms.find(room) == rooms.end()) {
                        sendToClient(clientSock, "Room does not exist. Use /create <room> to create it.");
                        continue;
                    }
                }
                string prev = removeClientFromRoom(clientSock);
                addClientToRoom(clientSock, room, false);

                string usernameLocal;
                {
                    lock_guard<mutex> lock(stateMutex);
                    usernameLocal = usernames[clientSock];
                }

                string joinMsg = usernameLocal + " joined the chat (" + to_string(roomCount(room)) + " online)";
                broadcastToRoom(room, joinMsg, clientSock);
                sendToClient(clientSock, "You joined room: " + room);
                writeGlobalLog("SERVER: " + usernameLocal + " joined room " + room);
                writeRoomLog(room, "SYSTEM: " + usernameLocal + " joined room");
                continue;
            }
            else if (cmd == "/leave") {
                string left = removeClientFromRoom(clientSock);
                if (left.empty()) {
                    sendToClient(clientSock, "You are not in any room.");
                } else {
                    string usernameLocal;
                    {
                        lock_guard<mutex> lock(stateMutex);
                        usernameLocal = usernames[clientSock];
                    }
                    string leaveMsg = usernameLocal + " left the chat (" + to_string(roomCount(left)) + " online)";
                    broadcastToRoom(left, leaveMsg, clientSock);
                    sendToClient(clientSock, "You left room: " + left);
                    writeGlobalLog("SERVER: " + usernameLocal + " left room " + left);
                    writeRoomLog(left, "SYSTEM: " + usernameLocal + " left room");
                }
                continue;
            }
            else if (cmd == "/listrooms") {
                sendRoomList(clientSock);
                continue;
            }
            else if (cmd == "/users") {
                string currentRoom;
                {
                    lock_guard<mutex> lock(stateMutex);
                    auto it = clientRoom.find(clientSock);
                    if (it != clientRoom.end()) currentRoom = it->second;
                }
                if (currentRoom.empty()) {
                    sendToClient(clientSock, "You are not in any room. Join a room to see its users.");
                } else {
                    sendUsersInRoom(clientSock, currentRoom);
                }
                continue;
            }
            else {
                sendToClient(clientSock, "Unknown command. Available: /create /join /leave /listrooms /users");
                continue;
            }
        }

        // Normal message: send to participants of current room
        string currentRoom;
        {
            lock_guard<mutex> lock(stateMutex);
            auto it = clientRoom.find(clientSock);
            if (it != clientRoom.end()) currentRoom = it->second;
        }
        if (currentRoom.empty()) {
            sendToClient(clientSock, "You are not in any room. Use /create or /join to enter a room.");
            continue;
        }

        string usernameLocal;
        {
            lock_guard<mutex> lock(stateMutex);
            usernameLocal = usernames[clientSock];
        }

        string fullMsg = usernameLocal + ": " + raw;
        cerr << "[" << currentRoom << "] " << fullMsg << "\n";

        broadcastToRoom(currentRoom, fullMsg, clientSock);
    }
}

int main() {
    // Prevent SIGPIPE from terminating the process when sending to closed sockets
    signal(SIGPIPE, SIG_IGN);
    int serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0) {
        cerr << "Failed to create socket\n";
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8000);
    addr.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(serverSock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        cerr << "Bind failed\n";
        close(serverSock);
        return 1;
    }

    if (listen(serverSock, 10) < 0) {
        cerr << "Listen failed\n";
        close(serverSock);
        return 1;
    }

    cout << "ConcurMeet Server (rooms enabled) running on port 8000\n";
    writeGlobalLog("SERVER STARTED (rooms enabled)");

    // Accept loop
    while (true) {
        int clientSock = accept(serverSock, nullptr, nullptr);
        if (clientSock < 0) {
            cerr << "Accept failed\n";
            continue;
        }
        // spawn thread
        thread t(handleClient, clientSock);
        t.detach();
    }

    close(serverSock);
    return 0;
}