#include "Server.h"
#include "Message.h"
#include "Uri.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h> 
#include <cerrno>
#include <chrono>
#include <cstring>
#include <functional>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <iostream>

void log(const std::string& message) {
    std::cout << message << std::endl;
}

namespace myHttpServer {


HttpServer::HttpServer(const std::string &host, std::uint16_t port)
    : host_(host),
      port_(port),
      sock_fd_(0),
      running_(false),
      workers(),
      rng_(std::chrono::steady_clock::now().time_since_epoch().count()),
      sleep_times_(10, 100) {
  CreateSocket();
}

void HttpServer::Start() {
    int opt = 1;
    sockaddr_in server_address;

    if (setsockopt(sock_fd_, SOL_SOCKET, SO_REUSEADDR, &opt,
                    sizeof(opt)) < 0) {
        throw std::runtime_error("Failed to set socket options");
    }
    
    // Bind the socket to the server address
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    inet_pton(AF_INET, host_.c_str(), &(server_address.sin_addr.s_addr));
    server_address.sin_port = htons(port_);

    if (bind(sock_fd_, (sockaddr *)&server_address, sizeof(server_address)) < 0) {
        throw std::runtime_error("Failed to bind to socket");
    }

    // Listen on the socket for incoming connections 
    if (listen(sock_fd_, kBacklogSize) < 0) {
        std::ostringstream msg;
        msg << "Failed to listen on port" << port_;
        throw std::runtime_error(msg.str());
    }

    // Create worker threads
    for (int i = 0; i < kThreadPoolSize; i++) {
        workers.emplace_back(&HttpServer::WorkerThread, this, i, sock_fd_);
    }

    // Set running_ to true
    running_ = true;
}

void HttpServer::Stop() {
    running_ = false;

    // join worker threads
    for (auto& worker : workers) {
        worker.join();
    }
    
    close(sock_fd_);
}

void HttpServer::CreateSocket() {
    if ((sock_fd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        throw std::runtime_error("Failed to create a TCP socket");
    }

    // Set the socket to non-blocking mode
    if (fcntl(sock_fd_, F_SETFL, O_NONBLOCK) < 0) {
        throw std::runtime_error("Failed to set the socket to non-blocking mode");
    }
}


void HttpServer::WorkerThread(int workerID, int listeningSocket) {
    EventData *clientData;
    EventData *data; 
    sockaddr_in clientAddress;
    socklen_t clientLen = sizeof(clientAddress);
    int clientSocket;
    int currentWorker = 0;
    bool active = true;

    struct timespec timeout;
    timeout.tv_sec = 3;  // 3 seconds
    timeout.tv_nsec = 0; // 0 nanoseconds

    // create a kqueue for this worker thread
    int kq = kqueue();
    if (kq == -1) {
        perror("kqueue");
        return;
    }

    // register the listening socket with the worker's kqueue
    ControlKqueueEvent(kq, EV_ADD, listeningSocket, EVFILT_READ);

    struct kevent events[kMaxEvents];
    while (running_) {
        // if there are no events to process, briefly sleep 
        if (!active) {
        std::this_thread::sleep_for(
            std::chrono::microseconds(sleep_times_(rng_)));
        }
        
        // monitor and retrieve events from kqueue 
        int numEvents = kevent(kq, nullptr, 0, events, kMaxEvents, &timeout);
        if (numEvents <= 0) {
            active = false;
            continue;
        }

        // iterate through the retrieved events and handle them 
        for (int i = 0; i < numEvents; ++i) {

            /////////////////////////////////////////////
            // Case 1: The event is a new connection 
            /////////////////////////////////////////////
            if (events[i].ident == listeningSocket) {
                // accept client socket 
                clientSocket = accept(listeningSocket, (sockaddr *)&clientAddress, &clientLen);
                if (clientSocket < 0) {
                    active = false;
                    continue;
                }
                else {
                    // set the accepted socket to non-blocking mode 
                    int flags = fcntl(clientSocket, F_GETFL, 0);
                    if (flags == -1) {
                        close(clientSocket);
                    }
                    else {
                        flags |= O_NONBLOCK;
                        if (fcntl(clientSocket, F_SETFL, flags) == -1) {
                            close(clientSocket);
                        }
                    }

                    clientData = new EventData();
                    clientData->fd = clientSocket;
    
                    // register the client socket with the worker's kqueue with a timeout of three seconds 
                    ControlKqueueEvent(kq, EV_ADD, clientSocket, EVFILT_READ, clientData, &timeout);
                    //std::cout << "Worker " << workerID << " accepted connection on socket " << clientSocket << std::endl;
                }
            }

            ////////////////////////////////////////////////////
            // Case 2: The event is a HTTP request or response 
            ///////////////////////////////////////////////////
            else {
                active = true; 
                const struct kevent &current_event = events[i];
                data = reinterpret_cast<EventData *>(current_event.udata);

                if (current_event.filter == EVFILT_READ) {
                    // handle read event
                    HandleKqueueEvent(kq, data, EVFILT_READ);
                } 
                else if (current_event.filter == EVFILT_WRITE) {
                    // handle write event
                    HandleKqueueEvent(kq, data, EVFILT_WRITE);
                } 
                else {
                    log("Unexpected event");
                    ControlKqueueEvent(kq, EV_DELETE, data->fd, current_event.filter);
                    delete data; 
                    close(data->fd);
                }
            }
        }
    }
}


void HttpServer::HandleKqueueEvent(int kq, EventData *data, int filter) {
    int fd = data->fd;
    EventData *request, *response;
    // read event 
    if (filter == EVFILT_READ) {
        request = data;
        ssize_t byte_count = recv(fd, request->buffer, kMaxBufferSize, 0); // read data from client

        // we have fully received the message TODO: implement a way to check if the full message has not been received 
        if (byte_count > 0) {  
            response = new EventData();
            response->fd = fd;
            HandleHttpData(*request, response); 
            send(fd, response->buffer + response->cursor, response->length, 0);
            ControlKqueueEvent(kq, EV_ADD, fd, EVFILT_WRITE, response);
            //delete request; 
        } 
    } 
    // write event 
    else if (filter == EVFILT_WRITE) {
        // eventually move send into here 

        if(!data->keepAlive) {
            delete data;
            close(fd);
        }
        else {
            std::cout << "connection is kept alive" << "\n";
        }
    }
}


// Takes in a HTTP request string and copies the resulting HTTP response into a string 
// and then copies it into an EventData response object
void HttpServer::HandleHttpData(const EventData &raw_request, EventData *raw_response) {
    std::string request_string(raw_request.buffer), response_string;
    std::vector<char> response_vector; 
    HttpRequest http_request;
    HttpResponse http_response;

    try {
        http_request = string_to_request(request_string);
        http_response = HandleHttpRequest(http_request);
    } 
    catch (const std::invalid_argument &e) {
        http_response = HttpResponse(HttpStatusCode::BadRequest);
        http_response.SetContent(e.what());
    } 
    catch (const std::logic_error &e) {
        http_response = HttpResponse(HttpStatusCode::HttpVersionNotSupported);
        http_response.SetContent(e.what());
    } 
    catch (const std::exception &e) {
        http_response = HttpResponse(HttpStatusCode::InternalServerError);
        http_response.SetContent(e.what());
    }

    response_vector = to_chars(http_response, true);
    size_t copy_length = std::min(response_vector.size(), static_cast<size_t>(kMaxBufferSize));
    memcpy(raw_response->buffer, response_vector.data(), copy_length);
    raw_response->length = static_cast<int>(copy_length);
    raw_response->keepAlive = http_response.GetKeepAlive();
}


// Takes in HTTP Request and returns a HTTP Response object
HttpResponse HttpServer::HandleHttpRequest(const HttpRequest &request) {
    std::cout << "The Request:" << to_string(request) << "\n";

    // find the handler for the request method 
    auto it = request_handlers_.find(request.method());
    if (it == request_handlers_.end()) {  // this method is not registered
        return HttpResponse(HttpStatusCode::NotFound);
    }

    return it->second(request);  // call handler to process the request
}


void HttpServer::ControlKqueueEvent(int kq, int op, int fd, std::uint32_t events, void *data, struct timespec *timeout) {
    struct kevent kev; 

    /*std::cout << "op " << op << "\n";
    std::cout << "fd " << fd << "\n";
    std::cout << "events " << events << "\n";
    std::cout << "data " << data << "\n";*/

    if (op == 2) {
        EV_SET(&kev, fd, events, EV_DELETE, 0, 0, data);
    } 
    else {
        EV_SET(&kev, fd, events, EV_ADD | EV_CLEAR, 0, 0, data);
    }

    // there is timeout associated with this event (new connection)
    if (timeout) {
        int res = kevent(kq, &kev, 1, &kev, 1, timeout);
        if (res == -1) {
            if (op == EV_DELETE && errno == ENOENT) {
                return; 
            }
            throw std::runtime_error((op == EV_DELETE) ? "Failed to remove file descriptor" : "Failed to add file descriptor");
        }
        else if (res == 0) {
            printf("Timeout! Connection closed. \n");
            close(fd);
        }
        else {
            // data ia available for reading on new connection, add read event 
            int res = kevent(kq, &kev, 1, NULL, 0, 0);
        }
    }
    // no timeout associated 
    else {
        int res1 = kevent(kq, &kev, 1, NULL, 0, 0);
        if (res1 == -1) {
            if (op == EV_DELETE && errno == ENOENT) {
                // ENOENT = non existent event 
                return; 
            }
            throw std::runtime_error((op == EV_DELETE) ? "Failed to remove file descriptor" : "Failed to add file descriptor");
        }
    }
}

}  // namespace myHttpServer