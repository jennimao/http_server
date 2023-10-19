// html server in c++
#include "Server.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>

#include "Message.h"
#include "Uri.h"

// Define the HttpServer namespace
namespace HttpServer {

    // Constructor
    HttpServer::HttpServer(const std::string& host, std::uint16_t port)
        : host_(host),
          port_(port),
          sock_fd_(0),
          running_(false),
          kq_(0),
          sleep_times_(10, 100) {
        CreateSocket();
        // SetUpKQueue();
    }

    // Destructor
    HttpServer::~HttpServer() {
        Stop();
    }

    // Create the server socket
    void HttpServer::CreateSocket() {
        // Create a socket
        sock_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd_ == -1) {
            throw std::runtime_error("Failed to create socket");
        }

        // Set socket options (optional)
        int opt = 1;
        if (setsockopt(sock_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
            throw std::runtime_error("Failed to set socket options");
        }

        // Bind the socket to a specific address and port
        struct sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = inet_addr(host_.c_str());
        serverAddress.sin_port = htons(port_);

        if (bind(sock_fd_, reinterpret_cast<struct sockaddr*>(&serverAddress), sizeof(serverAddress)) == -1) {
            throw std::runtime_error("Failed to bind the socket");
        }
    }

    // Set up the KQueue descriptor
    void HttpServer::SetUpKQueue() {
        // Implement KQueue setup logic
        // Example: kq_ = kqueue();
    }

    // Start the HTTP server
    void HttpServer::Start() {
        // Set the socket to listen for incoming connections
        if (listen(sock_fd_, kBacklogSize) == -1) {
            throw std::runtime_error("Failed to listen for connections");
        }

        // Create listener thread
        listener_thread_ = std::thread([this] { Listen(); });

        // Create and start worker threads
        for (int i = 0; i < kThreadPoolSize; ++i) {
            worker_threads_[i] = std::thread([this, i] { ProcessEvents(i); });
        }
    }

    // Stop the HTTP server: Close sockets, join threads 
    void HttpServer::Stop() {
        // Stop the listener thread
        if (listener_thread_.joinable()) {
            listener_thread_.join();
        }

        // Signal worker threads to stop (you need to implement a thread-safe mechanism for this)
        // For example, you can use condition variables or other synchronization primitives.
        // This code is just a placeholder; actual thread management may be more complex.
        for (int i = 0; i < kThreadPoolSize; ++i) {
            // Signal worker thread to exit (you need to implement this logic)
            // worker_threads_[i]. ... 

        }

        // Join worker threads (you need to implement thread joining logic)
        for (int i = 0; i < kThreadPoolSize; ++i) {
            if (worker_threads_[i].joinable()) {
                worker_threads_[i].join();
            }
        }

        // Close server socket
        close(sock_fd_);
    }

    // Listen for incoming connections
    void HttpServer::Listen() {
        struct sockaddr_in clientAddress;
        socklen_t clientAddressLength = sizeof(clientAddress);

        while (running_) {
            // Accept a new client connection
            int client_fd = accept(sock_fd_, reinterpret_cast<struct sockaddr*>(&clientAddress), &clientAddressLength);
            if (client_fd == -1) {
                // Handle the error or break the loop if you want to stop accepting connections
                if (!running_) {
                    break; // The server is stopping, exit the loop
                }
                // Handle the error (e.g., log it) or continue accepting connections
                continue;
            }

            // Handle the new client connection (you can create a new thread or use a worker pool)
            // For example, you can pass the client_fd to a worker thread for processing.
            // WorkerThread::ProcessClientConnection(client_fd);
        }
    }

    // Add a request handler
    void HttpServer::AddRequestHandler(const std::string& path, HttpMethod method, void (*handler)()) {
        // Implement request handler addition logic

        // Example: 
        request_handlers_[path].insert(std::make_pair(method, std::move(callback)));
        
    }

    // Process events in a worker thread --> CHANGE THIS to kqueue
    void HttpServer::ProcessEvents(int worker_id) {
        // Implement event processing logic for worker thread
        EventData *data;
        int epoll_fd = worker_epoll_fd_[worker_id];
        bool active = true;

        while (running_) {
            if (!active) {
                std::this_thread::sleep_for(
                    std::chrono::microseconds(sleep_times_(rng_)));
            }
            int nfds = epoll_wait(worker_epoll_fd_[worker_id],
                       worker_events_[worker_id], HttpServer::kMaxEvents, 0);
            if (nfds <= 0) {
            active = false;
            continue;
            }

            active = true;
            for (int i = 0; i < nfds; i++) {
            const epoll_event &current_event = worker_events_[worker_id][i];
            data = reinterpret_cast<EventData *>(current_event.data.ptr);
            if ((current_event.events & EPOLLHUP) ||
                (current_event.events & EPOLLERR)) {
                control_epoll_event(epoll_fd, EPOLL_CTL_DEL, data->fd);
                close(data->fd);
                delete data;
            } else if ((current_event.events == EPOLLIN) ||
                        (current_event.events == EPOLLOUT)) {
                HandleEpollEvent(epoll_fd, data, current_event.events);
            } else {  // something unexpected
                control_epoll_event(epoll_fd, EPOLL_CTL_DEL, data->fd);
                close(data->fd);
                delete data;
            }
            }
        }
    }

    // Handle a KQueue event
    void HttpServer::HandleKQueueEvent(int kq_fd, EventData* event, std::uint32_t events) {
        // Implement event handling logic for KQueue
    }

    // Handle HTTP data by parsing request and returning the response from handler
    void HttpServer::HandleHttpData(const EventData& request, EventData* response) {
        std::string request_string(request.buffer), response_string;
        HttpRequest http_request;
        HttpResponse http_response;

        try {
            http_request = string_to_request(request_string);
            http_response = HandleHttpRequest(http_request);
        } catch (const std::invalid_argument &e) {
            http_response = HttpResponse(HttpStatusCode::BadRequest);
            http_response.SetContent(e.what());
        } catch (const std::logic_error &e) {
            http_response = HttpResponse(HttpStatusCode::HttpVersionNotSupported);
            http_response.SetContent(e.what());
        } catch (const std::exception &e) {
            http_response = HttpResponse(HttpStatusCode::InternalServerError);
            http_response.SetContent(e.what());
        }

        response_string = to_string(http_response, http_request.method() != HttpMethod::HEAD);
        memcpy(response->buffer, response_string.c_str(), kMaxBufferSize);
        // will need to turn this into a chunked response 
        response->length = response_string.length();
    }

    // Handle an HTTP request by calling the handler for the request 
    HttpResponse HttpServer::HandleHttpRequest(const HttpRequest& request) {
        HttpMethod method = request.method();
        std::string path = request.uri();

        // Search for a handler associated with the URI and HTTP method
        auto it = request_handlers_.find(path);
        if (it != request_handlers_.end()) {
            auto callback_it = it->second.find(method);
            if (callback_it != it->second.end()) {
                return callback_it->second(request);
            }
        }

        return HttpResponse(HttpStatusCode::NotFound);
    }

    // Control KQueue event
    void HttpServer::control_kqueue_event(int kq_fd, int op, int fd,
                                          std::uint32_t events, void* data) {
        // Implement control of KQueue events
    }

}  // End of HttpServer namespace