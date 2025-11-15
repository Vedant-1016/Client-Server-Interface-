#include <iostream>
#include <thread>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
using namespace std;

void receiveMessages(int sock) {
    char buffer[1024];

    while (true) {
        int bytes = recv(sock, buffer, 1024, 0);

        if (bytes <= 0) {
            cout << "Disconnected from server." << endl;
            close(sock);
            break;
        }

        buffer[bytes] = '\0';
        cout << "\n[Server]: " << buffer << endl;
        cout << "> " << flush;   // show prompt again
    }
}

int main() {

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        cout << "Socket creation failed!" << endl;
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8000);

    // connect to localhost server
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cout << "Connection to server failed!" << endl;
        return 1;
    }

    cout << "Connected to ConcurMeet Server!" << endl;

    string username;
    cout << "Enter your username: ";
    getline(cin, username);
    send(clientSocket, username.c_str(), username.size(), 0);

    // create thread to receive messages
    thread recvThread(receiveMessages, clientSocket);
    recvThread.detach();

    string msg;
    while (true) {
        cout << "> ";
        getline(cin, msg);

        if (cin.eof()) { // User pressed Ctrl+D (Unix) or Ctrl+Z (Windows) to signal EOF
            cout << "Exiting client.\n";
            break;
        }

        if (send(clientSocket, msg.c_str(), msg.size(), 0) == -1) {
            cerr << "Failed to send message.\n";
            break; // Exit loop if sending fails
        }
    }
 
    close(clientSocket);
    return 0;
}
