#include <iostream>
#include <string>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

void log(const std::string& message) {
    std::cout << message << std::endl;
}

class Client {
public:
    Client(boost::asio::io_context& io_context, const std::string& host, const std::string& port)
        : io_context_(io_context), resolver_(io_context), socket_(io_context) {
        server_host_ = host;
        server_port_ = port;
    }

    void sendGetRequest(const std::string& path) {
        tcp::resolver::results_type endpoints = resolveHost();

        if (endpoints.empty()) {
            std::cerr << "No endpoints found." << std::endl;
            return;
        }

        std::cout << "Resolved endpoints:" << std::endl;
        for (const auto& endpoint : endpoints) {
            std::cout << endpoint.endpoint().address().to_string() << ":" << endpoint.endpoint().port() << std::endl;
        }

        connectToServer(endpoints);
        sendRequest("GET", path);
        readResponse();
    }

private:
    boost::asio::io_context& io_context_;
    tcp::resolver resolver_;
    tcp::socket socket_;
    std::string server_host_;
    std::string server_port_;

    tcp::resolver::results_type resolveHost() {
        return resolver_.resolve(server_host_, server_port_);
    }

    void connectToServer(tcp::resolver::results_type endpoints) {
        boost::system::error_code ec;
        boost::asio::connect(socket_, endpoints, ec);

        if (ec) {
            std::cerr << "Failed to connect to the server: " << ec.message() << std::endl;
            // Handle the connection error here, if needed
        } else {
            std::cout << "Connected to the server" << std::endl;
        }
    }

    void sendRequest(const std::string& method, const std::string& path) {
        std::string request =
            method + " " + path + " HTTP/1.1\r\n" +
            "Host: " + "mobile.cicada.cs.yale.edu" + "\r\n" +
            "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/15.0 Safari/605.1.15" + "\r\n" +
            "Authorization: Basic aGVsbG86d29ybGQ=" + "\r\n" +
            "If-Modified-Since: Mon, 23 Oct 2023 20:23:00 GMT" + "\r\n" +
            "Connection: close\r\n\r\n";
        std::size_t bytes_transferred = boost::asio::write(socket_, boost::asio::buffer(request));

        if (bytes_transferred == request.size()) {
            std::cout << "Request sent successfully" << std::endl;
        } else {
            std::cerr << "Failed to send the complete request" << std::endl;
            // Handle the failure if needed
        }
    }

    void readResponse() {
        boost::asio::streambuf response;
        boost::asio::read_until(socket_, response, "\r\n");

        std::istream response_stream(&response);
        std::string http_version;
        response_stream >> http_version;
        unsigned int status_code;
        response_stream >> status_code;
        std::string status_message;
        std::getline(response_stream, status_message);

        if (!response_stream || http_version.substr(0, 5) != "HTTP/") {
            std::cerr << "Invalid response\n";
            return;
        }

        std::cout << "Response status code: " << status_code << "\n";
        std::cout << "Response status message: " << status_message << "\n";
        std::cout << "Response HTTP version: " << http_version << "\n";

        boost::asio::read_until(socket_, response, "\r\n\r\n");
        std::cout << "Response headers:\n";
        std::cout << &response;

        boost::system::error_code error;
        while (boost::asio::read(socket_, response, boost::asio::transfer_at_least(1), error)) {
            std::cout << "Received data:\n";
            std::cout << &response;
        }

        if (error != boost::asio::error::eof) {
            std::cerr << "Error reading response: " << error.message() << "\n";
            throw boost::system::system_error(error);
        }
    }

};

int main() {
    boost::asio::io_context io_context;
    Client client(io_context, "localhost", "8080");
    
    for (int i = 0; i < 10; i++) {
        client.sendGetRequest("/");
    }
    // client.sendGetRequest("/goodbyeWorld_m.html");

    return 0;
}






