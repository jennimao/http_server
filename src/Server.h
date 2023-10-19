#ifndef SERVER_H_
#define SERVER_H_

#include <string>
#include <cstdint>
#include <thread>
#include <vector>
#include <map>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

namespace HttpServer {

    // Maximum size of an HTTP message is limited by how much bytes
    // we can read or send via socket each time
    constexpr size_t kMaxBufferSize = 4096;

    // server is made up of: 
    //      management thread to display control terminal 
    //      listener thread to accept connections
    //      worker threads to handle requests 
    class HttpServer {
        public:
            explicit HttpServer(const std::string& host, std::uint16_t port);
            ~HttpServer();
            HttpServer() = default;
            
            void Start();
            void Stop();

            // add a request handler 
            void AddRequestHandler(const std::string& path, HttpMethod method,
                                            void (*handler)()) {
                // Uri uri(path); // if you decide to make a URI class
                // request_handlers_[uri].insert(std::make_pair(method, std::move(callback)));
            }

            /* std::string host() const { return host_; } --> if these are defined in constructor, do i need? 
            std::uint16_t port() const { return port_; }
            bool running() const { return running_; } */

        // all the server methods 
        private:
            static constexpr int kBacklogSize = 1000;
            static constexpr int kMaxConnections = 10000;
            static constexpr int kMaxEvents = 10000;
            static constexpr int kThreadPoolSize = 5;

            std::string host_;
            std::uint16_t port_;
            int sock_fd_;
            bool running_;
            std::thread listener_thread_;
            std::thread worker_threads_[kThreadPoolSize];
            int worker_kq_fd_[kThreadPoolSize];
            int kq_; 
            // map that stores request handlers associated with a URI and HTTP method
            std::map<Uri, std::map<HttpMethod, HttpRequestHandler_t>> request_handlers_;
            std::uniform_int_distribution<int> sleep_times_;

            void CreateSocket();
            void SetUpEpoll();
            void Listen();
            void ProcessEvents(int worker_id);
            void HandleKQueueEvent(int kq_fd, EventData* event, std::uint32_t events);
            void HandleHttpData(const EventData& request, EventData* response);
            HttpResponse HandleHttpRequest(const HttpRequest& request); //TODO: fix 

            void control_kqueue_event(int kq_fd, int op, int fd,
                                    std::uint32_t events = 0, void* data = nullptr);
    };

}

#endif // SERVER_H_ 
