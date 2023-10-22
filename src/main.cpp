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

void printAllFields(HTTPRequest* request);

int main() {

    //test
    std::string myString  = "GET /resource/path HTTP/1.1\r\nHost: example.com\r\nAccept: text/html, application/json\r\nUser-Agent: MyUserAgent/1.0\r\nIf-Modified-Since: Tue, 15 Nov 2022 08:12:31 GMT\r\nConnection: close\r\nAuthorization: Bearer YourAccessToken\r\n\r\n";
    HTTPRequest myOtherRequest;

    string_to_request(myString, &myOtherRequest);
    printAllFields(&myOtherRequest);


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
   
    struct sockaddr_in clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);
    int clientSocket; 
    // Accept incoming connections and handle client requests
    while (true) {
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);
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

            std::cout << buffer;

            //ParsingTest
            HTTPRequest myRequest;
            int ret = string_to_request(buffer, &myRequest);
            std::cout << "Ret" << ret << "\n";
            printAllFields(&myRequest);


            // std::cout << buffer;


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

void printAllFields(HTTPRequest* request)
{
    std::cout << "PRINT ALL FIELDS\n";
    //method
    if(request->method == MethodType::GET)
    {
        std::cout << "Method: GET\n";
    }
    else if(request->method == MethodType::POST)
    {
        std::cout << "Method: POST\n";
    }
    else
    {
        std::cout << "Method: Bad\n";
    }

    //Target
    std::cout << "Target:" <<request->target.getStr() << "\n";

    //Version
    if(request->version == Version::HTTP_1_1)
    {
       std::cout << "Version: 1.1\n";
    }
    else
    {
        std::cout << "Version: BAD\n";
    }

    //Virtual Host
    std::cout << "VirtualHost:" <<request->host.getStr() << "\n";

    //Headers
    std::cout << "AcceptHeader:" <<request->headers.accept.getStr() << "\n";
    std::cout << "UserAgentHeader:" <<request->headers.user_agent.getStr() << "\n";
    std::cout << "IfModifiedSince:" <<request->headers.modified.getStr() << "\n";
    std::cout << "ConnectionHeader:" <<request->headers.connection.getStr() << "\n";
    std::cout << "AuthorizationHeader:" <<request->headers.authorization.getStr() << "\n";

    return;
}