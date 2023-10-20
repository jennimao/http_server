// RequestHandlerRegistry.h
#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include "Server.h"
#include "Uri.h"
#include "Message.h"


namespace Server {
    class RequestHandlerRegistry {
        public:
            static void RegisterHandlers(Server::HttpServer& server);
    };
}

#endif