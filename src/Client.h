#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <boost/asio.hpp>

class Client {
public:
    Client(boost::asio::io_context& io_context, const std::string& host, const std::string& port);

    void sendGetRequest(const std::string& path);

private:
    boost::asio::io_context& io_context;
    boost::asio::ip::tcp::resolver resolver;
    boost::asio::ip::tcp::socket socket;
    std::string server_host;
    std::string server_port;

    boost::asio::ip::tcp::resolver::results_type resolveHost();
    void connectToServer(boost::asio::ip::tcp::resolver::results_type endpoints);
    void sendRequest(const std::string& method, const std::string& path);
    void readResponse();

};

#endif // CLIENT_H
