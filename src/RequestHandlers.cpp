// RequestHandlerRegistry.cpp
#include "RequestHandlers.h"
#include "Server.h"
#include "Message.h"
#include "Uri.h"


namespace simple_http_server {

HttpResponse RequestHandlers::HandleGetRequest(const HttpRequest& request) {
    // Handle GET requests here and return an appropriate response
    if (request.headers.find("Host") == request.headers.end()) {
        HttpResponse response;
        response.status = HttpStatusCode::BadRequest;
        response.content = "400 Bad Request: Host header is missing";
        return response;
    }

    // Extract the URL and other headers from the request
    std::string url = request.url;
    std::string host = request.headers.at("Host");

    // Process the request and generate a response
    HttpResponse response;
    response.status = HttpStatusCode::Ok;
    response.content = "Response for GET request to URL: " + url + "\n";
    response.content += "Host: " + host + "\n";

    // Process other headers here if needed
    for (const auto& header : request.headers) {
        if (header.first != "Host") {
            response.content += header.first + ": " + header.second + "\n";
        }
    }

    return response;
}

HttpResponse RequestHandlers::HandlePostRequest(const HttpRequest& request) {
    // Handle POST requests here and return an appropriate response
    HttpResponse response;
    // ...
    return response;
}

void RequestHandlers::RegisterHandlers(HttpServer& server) {


    // Register GET and POST request handlers
    server.RegisterHttpRequestHandler(HttpMethod::GET, "<GET_URL>", RequestHandlers::HandleGetRequest);
    server.RegisterHttpRequestHandler(HttpMethod::POST, "<POST_URL>", RequestHandlers::HandlePostRequest);

    // Register all 50 request handlers here
}
}
