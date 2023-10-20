// RequestHandlerRegistry.h
#ifndef REQUEST_HANDLER_REGISTRY_H
#define REQUEST_HANDLER_REGISTRY_H

#include "Server.h" // Include your server's header file

class RequestHandlerRegistry {
public:
    static void RegisterHandlers(HttpServer& server);
};

#endif