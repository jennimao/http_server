// RequestHandlerRegistry.cpp
#include "RequestHandlers.h"
#include "Server.h"
#include "Message.h"
#include "Uri.h"


namespace simple_http_server {


HttpRequestHandler_t SayHelloHandler(const HttpRequest& request) {
    HttpResponse response(HttpStatusCode::Ok);
    response.SetHeader("Content-Type", "text/plain");
    response.SetContent("Hello, world\n");
    return response;
}


HttpResponse RequestHandlers::RegisterGetHandlers(const HttpServer& server) {
  // Define and register GET handlers here
    auto say_hello = [](const HttpRequest& request) -> HttpResponse {
        HttpResponse response(HttpStatusCode::Ok);
        response.SetHeader("Content-Type", "text/plain");
        response.SetContent("Hello, world\n");
        return response;
    };

    auto send_html = [](const HttpRequest& request) -> HttpResponse {
        HttpResponse response(HttpStatusCode::Ok);
        std::string content;
        content += "<!doctype html>\n";
        content += "<html>\n<body>\n\n";
        content += "<h1>Hello, world in an Html page</h1>\n";
        content += "<p>A Paragraph</p>\n\n";
        content += "</body>\n</html>\n";

        response.SetHeader("Content-Type", "text/html");
        response.SetContent(content);
    
        return response;
    };

    server.RegisterHttpRequestHandler("/", HttpMethod::GET, SayHelloHandler);
    server.RegisterHttpRequestHandler("/hello.html", HttpMethod::GET, send_html);
}


void RequestHandlers::RegisterHandlers(HttpServer& server) {

    // Register GET and POST request handlers
    RegisterGetHandlers(server);

    // Register all 50 request handlers here
}

}
