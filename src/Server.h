// Defines the HTTP server object with some constants and structs
// useful for request handling and improving performance

#ifndef HTTP_SERVER_H_
#define HTTP_SERVER_H_

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <chrono>
#include <functional>
#include <map>
#include <random>
#include <string>
#include <thread>
#include <utility>

#include "Message.h"
#include "Uri.h"

namespace myHttpServer {

// Maximum size of an HTTP message is limited by how much bytes
// we can read or send via socket each time
constexpr size_t kMaxBufferSize = 16384;

struct EventData {
  EventData() : fd(0), length(0), cursor(0), buffer() {}
  int fd;
  size_t length;
  size_t cursor;
  char buffer[kMaxBufferSize];
  bool keepAlive;
};


// A request handler should expect a request as argument and returns a response
//using HttpRequestHandler_t = std::function<HttpResponse(const HttpRequest&)>;
using HttpRequestHandler_t = std::function<HttpResponse(const HttpRequest&)>;


// The server consists of:
// - 1 main thread
// - 1 listener thread that is responsible for accepting new connections
// - Many worker threads that process HTTP messages and communicate with
// clients via socket.
//   The number of workers is defined by a constant
class HttpServer {
    public:
        explicit HttpServer(const std::string& host, std::uint16_t port);
        ~HttpServer() = default;

        HttpServer() = default;
        HttpServer(HttpServer&&) = default;
        HttpServer& operator=(HttpServer&&) = default;

        void Start();
        void Stop();

        void RegisterHttpRequestHandler(MethodType method, const HttpRequestHandler_t callback) {
            request_handlers_[method] = std::move(callback);
        }

        std::string host() const { return host_; }
        std::uint16_t port() const { return port_; }
        bool running() const { return running_; } 
        int sock_fd_;

    private:
        static constexpr int kBacklogSize = 1000;
        static constexpr int kMaxConnections = 500;
        static constexpr int kMaxEvents = 100;
        static constexpr int kThreadPoolSize = 4;

        std::string host_;
        std::uint16_t port_;
        bool running_;
        std::thread listener_thread_;
        std::vector<std::thread> workers;
        //std::thread worker_threads_[kThreadPoolSize];
        int worker_kqueue_fd_[kThreadPoolSize];
        struct kevent worker_events[kMaxEvents];
        //std::map<Uri, std::map<MethodType, HttpRequestHandler_t>> request_handlers_;
        std::map<MethodType, HttpRequestHandler_t> request_handlers_;
        std::mt19937 rng_;
        std::uniform_int_distribution<int> sleep_times_;

        void CreateSocket();
        void WorkerThread(int workerID, int listeningSocket);
        void HandleKqueueEvent(int kq, EventData *data, int filter);
        void HandleHttpData(const EventData& request, EventData* response);
        HttpResponse HandleHttpRequest(const HttpRequest& request);
        void ControlKqueueEvent(int kq, int op, int fd,
                                std::uint32_t events = 0, void *data = nullptr, 
                                struct timespec *timeout = nullptr);
    };

}  // namespace myHttpServer

#endif  // HTTP_SERVER_H_