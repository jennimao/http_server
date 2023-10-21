#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> 
#include "Message_new.h"

/* struct HttpRequest {
    std::string method;
    std::string uri;
    std::string headers;
    std::string body;
};

// Function to parse an HTTP request
HttpRequest ParseHttpRequest(int clientSocket) {
    HttpRequest request;
    char buffer[8192]; // Adjust buffer size as needed

    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesRead == -1) {
        std::cerr << "Error reading from client" << std::endl;
        return request;
    }

    std::string requestStr(buffer, bytesRead);

    // Simple parsing: Split the request into lines and extract method, URI, headers, and body
    size_t endOfHeaders = requestStr.find("\r\n\r\n");
    if (endOfHeaders != std::string::npos) {
        size_t endOfFirstLine = requestStr.find("\r\n");
        if (endOfFirstLine != std::string::npos) {
            request.method = requestStr.substr(0, endOfFirstLine);
            requestStr = requestStr.substr(endOfFirstLine + 2);

            size_t endOfURI = requestStr.find(" ");
            if (endOfURI != std::string::npos) {
                request.uri = requestStr.substr(0, endOfURI);
            }

            request.headers = requestStr;
        }
    }

    return request;
} */

int main() {
    // Create a socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Error creating socket" << std::endl;
        return 1;
    }

    // Bind the socket to an IP address and port
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8080);
    
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Error binding socket" << std::endl;
        return 1;
    }

    // Set the socket to listen for incoming connections
    if (listen(serverSocket, 10) == -1) {
        std::cerr << "Error listening on socket" << std::endl;
        return 1;
    }

    std::cout << "hello1\n";
    struct sockaddr_in clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);
    int clientSocket; 
    std::cout << "hello2\n";
    // Accept incoming connections and handle client requests
    while (true) {
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);
        std::cout << "hello\n";
        if (clientSocket == -1) {
            std::cerr << "Error accepting client connection" << std::endl;
        } else {
           
            char buffer[8192]; 
            ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesRead == -1) 
            {
                std::cerr << "Error reading from client" << std::endl;
                break;
            }

            HTTPRequest myRequest;
            string_to_request(buffer, &myRequest);
            if(myRequest.method == MethodType::POST)
            {
                std::cout << "Ok method \n";
            }

            std::cout << "Target: " << myRequest.target.getTarget() << "\n";

            if(myRequest.version == Version::HTTP_1_1)
            {
                std::cout << "Ok version \n";
            }


            std::cout << buffer;


            // Handle the request, e.g., based on the method and URI.

            close(clientSocket);
        }
        // Handle the client's requests in a new thread or process
        // (concurrent request handling logic goes here)
    }

    // Close the server socket when done
    close(serverSocket);

    return 0;
}