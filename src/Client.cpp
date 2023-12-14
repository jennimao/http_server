#include <iostream>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

// Include necessary headers for networking
#include <unistd.h>

class HTTPClient {
public:
    HTTPClient() {}

    void sendGetRequest(int sockfd, const std::string& path) {
        // Create an HTTP GET request
        std::string request = "GET " + path + " HTTP/1.1\r\n";
        request += "Host: cicada.cs.yale.edu\r\n"; // Replace with your server's host
        request += "User-Agent: CustomHTTPClient/1.0\r\n";
        request += "If-Modified-Since: Mon, 24 Oct 2023 13:24:40 GMT\r\n\r\n";
        //request += "Connection: close\r\n\r\n";
        printf("Request: %s\n", request.c_str());
        // Send the request to the server
        if (sendRequest(sockfd, request)) {
            std::string response = receiveResponse(sockfd);
            std::cout << "Received Response:\n" << response << std::endl;
        } else {
            std::cerr << "Failed to send the request or receive a response." << std::endl;
        }
    }

private:
    bool sendRequest(int sockfd, const std::string& request) {
        return write(sockfd, request.c_str(), request.length()) != -1;
    }

    std::string receiveResponse(int sockfd) {
        std::string response;
        char buffer[1024];
        int bytes_received;

        while ((bytes_received = read(sockfd, buffer, sizeof(buffer))) > 0) {
            response.append(buffer, bytes_received);
        }

        return response;
    }
};

int main() {
    int sockfd;
    struct sockaddr_in serverAddr;
    socklen_t addrSize;

    // Create a socket and connect to the server
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Replace with your server's IP address

    connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    HTTPClient client;


    client.sendGetRequest(sockfd, "/");

    client.sendGetRequest(sockfd, "/goodbyeWorld_m.html");
    
    close(sockfd);

    return 0;
}
