#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <unistd.h>
#include <sys/socket.h>   // <-- REQUIRED
#include <netinet/in.h>   // <-- REQUIRED
#include <arpa/inet.h>    // <-- REQUIRED
using namespace std;

vector<int> clients;          // list of connected client sockets
mutex clientMutex;            // protects clients vector

void broadcastMessage(const string &msg) {
    lock_guard<mutex> lock(clientMutex);
    for (int client : clients) {
        send(client, msg.c_str(), msg.size(), 0);
    }
}

void handleClient(int clientSocket) {
    char buffer[1024];

    while (true) {
        int bytes = recv(clientSocket, buffer, 1024, 0);
        if (bytes <= 0) {
            cout << "Client disconnected.\n";
            close(clientSocket);
            break;
        }

        buffer[bytes] = '\0';

        string message = "User says: " + string(buffer);
        cout << message << endl;

        broadcastMessage(message);
    }
}

int main() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8000);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, 10);

    cout << "ConcurMeet Server running on port 8000\n";

    while (true) {
        int clientSocket = accept(serverSocket, NULL, NULL);

        {
            lock_guard<mutex> lock(clientMutex);
            clients.push_back(clientSocket);
        }

        cout << "New client connected.\n";
        thread(handleClient, clientSocket).detach();
    }

    return 0;
}
