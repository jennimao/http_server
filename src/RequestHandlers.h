// RequestHandlers.h
#ifndef REQUEST_HANDLERS_H
#define REQUEST_HANDLERS_H

#include "Server.h" 
#include "Uri.h"
#include "Message.h"

namespace simple_http_server {

class RequestHandlers { 
public:
    static void RegisterHandlers(HttpServer& server);
    static void RegisterGetHandlers(HttpServer& server);
    static void RegisterPostHandlers(HttpServer& server);
    static void ParseConfigFile(std::string configfile, int* port, int* selectLoops);
    static void PrintVirtualHosts(void);
private:
    

};

} // namespace simple_http_server

#endif // REQUEST_HANDLERS_H
