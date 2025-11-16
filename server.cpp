// #include <iostream>
// #include <thread>
// #include <vector>
// #include <mutex>
// #include <algorithm> // Added for std::remove
// #include <unistd.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
// #include <map> // Added for std::map

// using namespace std;

// vector<int> clients;          
// mutex clientMutex;            
// map<int, string> clientUsernames; // Map to store client usernames

// void broadcastMessage(const string &msg) {
//     lock_guard<mutex> lock(clientMutex);
//     for (int client : clients) {
//         if (send(client, msg.c_str(), msg.size(), 0) == -1) {
//             cerr << "Failed to send message to client " << client << ".\n";
//             // Optionally, handle error, e.g., remove client from list if send fails consistently
//         }
//     }
// }

// void handleClient(int clientSocket) {
//     char buffer[1024];
//     string username;

//     // Receive username
//     int bytes = recv(clientSocket, buffer, 1024, 0);
//     if (bytes <= 0) {
//         cout << "Client disconnected during username reception.\n";
//         close(clientSocket);
//         lock_guard<mutex> lock(clientMutex);
//         clients.erase(remove(clients.begin(), clients.end(), clientSocket), clients.end());
//         return; // Exit if username not received
//     }
//     buffer[bytes] = '\0';
//     username = string(buffer);
//     {
//         lock_guard<mutex> lock(clientMutex);
//         clientUsernames[clientSocket] = username;
//     }
//     cout << "New client connected: " << username << endl;
//     broadcastMessage(username + " has joined the chat.\n");

//     while (true) {
//         bytes = recv(clientSocket, buffer, 1024, 0);
//         if (bytes <= 0) {
//             cout << "Client " << username << " disconnected.\n";
//             close(clientSocket);

//             // remove from list and map
//             lock_guard<mutex> lock(clientMutex);
//             clients.erase(remove(clients.begin(), clients.end(), clientSocket), clients.end());
//             clientUsernames.erase(clientSocket);
//             broadcastMessage(username + " has left the chat.\n");
//             break;
//         }

//         buffer[bytes] = '\0';
//         string message = username + ": " + string(buffer);
//         cout << message << endl;

//         broadcastMessage(message);
//     }
// }

// int main() {

//     // CREATE SOCKET
//     int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
//     if (serverSocket == -1) {
//         cerr << "Failed to create socket.\n";
//         return 1;
//     }

//     // SERVER ADDRESS
//     sockaddr_in serverAddr;
//     serverAddr.sin_family = AF_INET;
//     serverAddr.sin_port = htons(8000);
//     serverAddr.sin_addr.s_addr = INADDR_ANY;

//     // BIND
//     if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
//         cerr << "Failed to bind to port.\n";
//         close(serverSocket);
//         return 1;
//     }

//     // LISTEN
//     if (listen(serverSocket, 10) == -1) {
//         cerr << "Failed to listen on socket.\n";
//         close(serverSocket);
//         return 1;
//     }

//     cout << "ConcurMeet Server running on port 8000\n";

//     while (true) {
//         int clientSocket = accept(serverSocket, NULL, NULL);
//         if (clientSocket == -1) {
//             cerr << "Failed to accept client connection.\n";
//             continue;
//         }

//         lock_guard<mutex> lock(clientMutex);
//         clients.push_back(clientSocket);

//         cout << "New client connected.\n";
//         thread(handleClient, clientSocket).detach();
//     }

//     // The server socket is never explicitly closed because the main loop is infinite.
//     // In a real-world scenario, you might have a mechanism to gracefully shut down.
//     return 0;
// }
