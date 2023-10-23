// RequestHandlerRegistry.cpp
#include "RequestHandlers.h"
#include "Server.h"
#include "Message.h"
#include "Uri.h"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>


namespace simple_http_server {
std::unordered_map<std::string, std::string> virtualHosts;


HttpResponse SayHelloHandler(const HttpRequest& request) {
    HttpResponse response(HttpStatusCode::Ok);
    response.SetHeader("Content-Type", "text/plain");
    response.SetContent("Hello, world\n");
    return response;
}

void fillInBadResponse(HttpResponse& response)
{
    return;
}


HttpResponse GetHandler(const HttpRequest& request) 
{
    HttpResponse ourResponse;
    //validate URI
    std::string our_uri = request.uri().path();
     if(our_uri[0] != '/' || our_uri[0] != 'h')
    {
        std::cerr << "Bad URL\n";
        fillInBadResponse(&ourResponse);
        return ourResponse;
    }
    else if(our_uri.find("..") != std::string::npos)
    {
        std::cerr << "Bad URL\n";
        fillInBadResponse(&ourResponse);
        return ourResponse;
    }

    //Accept Header
    if(request.headers().find("Accept") != request.headers().end())
    {
        //parse accept, we only handle text/html, text/plain for now
        std::vector<std::string> acceptedValues = {"text/html", "text/plain"};
        std::string values = request.headers()["Accept"];
        std::stringstream ss(values);

        std::vector<std::string> tokens;
        std::string token;

        while (std::getline(ss, token, ',')) {
            tokens.push_back(token); //tokens has all of the accepted formats
        }

        for (const std::string& token : tokens) {
           if(token.compare(acceptedValues[0]) == 0) //could eventually turn this into a for
           {
                //add HTML to some sort of content generation array??
           }
           else if (token.compare(acceptedValues[1]) == 0)
           {
               //add plain to some sort of content generation array??
           }
           //if its not one of the type we handle, ignore
        }

    }

    //User-Agent
    if(request.headers().find("UserAgent") != request.headers().end())
    {
        //right now we only handle mobile user or not
        std::string user_agent = request.headers()["UserAgent"];
        if(user_agent.find("Mobile") != std::string::npos || 
           user_agent.find("Andriod") != std::string::npos || 
           user_agent.find("iOS") != std::string::npos || 
           user_agent.find("iPhone") != std::string::npos
        )
        {
            //add mobile agent to content gen array
        }
        else
        {
            //add not mobile to some sort of content generation array
        }

    }

     if(request.headers().find("If-Modified-Since") != request.headers().end())
    {
        //parsing the Http time/date format
        std::istringstream ss(request.headers()["If-Modified-Since"]);
        std::tm timeInfo = {};
        ss.imbue(std::locale("C")); // Set locale to "C" for consistent parsing

        ss >> std::get_time(&timeInfo, "%a, %d %b %Y %H:%M:%S GMT");
        if (ss.fail()) {
            throw std::runtime_error("Failed to parse HTTP-formatted time.");
        }

        std::time_t time = std::mktime(&timeInfo);
        std::chrono::system_clock::time_point compareable_time = std::chrono::system_clock::from_time_t(time);
    }



    //

    }
    
   

    //construct a filepath
    //ensure the end of the path is a valid file
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

    //auto getResponse = 

    server.RegisterHttpRequestHandler("/", HttpMethod::GET, SayHelloHandler);
    server.RegisterHttpRequestHandler("/hello.html", HttpMethod::GET, send_html);
}


void RequestHandlers::RegisterHandlers(HttpServer& server) {

    // Register GET and POST request handlers
    RegisterGetHandlers(server);

    // Register all 50 request handlers here
}

}
