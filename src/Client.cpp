#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

class Client {
public:
    Client(const std::string& serverAddress, int serverPort) 
        : serverAddress_(serverAddress), serverPort_(serverPort) {
    }

    void connectAndSend(const std::string& message) {
        // Create a socket
        int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == -1) {
            std::cerr << "Failed to create socket" << std::endl;
            return;
        }

        // Set up server address
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(serverPort_);
        serverAddr.sin_addr.s_addr = inet_addr(serverAddress_.c_str());

        // Connect to the server
        if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
            std::cerr << "Failed to connect to the server" << std::endl;
            close(clientSocket);
            return;
        }

        // Send the message to the server
        send(clientSocket, message.c_str(), message.size(), 0);
        std::cout << message << std::endl;

        // Close the socket
        close(clientSocket);
    }

private:
    std::string serverAddress_;
    int serverPort_;
};

int main() {
    Client client("localhost", 8080);
    client.connectAndSend("Hello, Server!");
    
    return 0;
}