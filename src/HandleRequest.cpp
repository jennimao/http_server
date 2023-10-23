#include "Message_new.h"

//steps to handling a HTTP request
//1. Parse the message + headers
//2. Decide if it is POST or GET

//If GET:
//identify the resource
//checking permissions
//retrieve resource
//generating response headers
//sending response
//close or not(based on header)
enum ResponseCodes {
    BAD_RESPONSE,
    OK
};

std::unordered_map<std::string, std::string> virtualHosts;

int handleParsedRequest(HTTPRequest* request)
{
    if(request->method == MethodType::GET)
    {
        handleParsedGET(request);
    }
}

int handleParsedGET(HTTPRequest* request)
{
    std::string filepath;
    //match host with its root filepath
    if(virtualHosts.find(request->host.getStr()) != virtualHosts.end())
    {
        filepath = virtualHosts[request->host.getStr()];
    }
    else
    {
        std::cerr << "Bad host\n";
        return BAD_RESPONSE;
    }

    //validate integrity of URL
    std::string url = request->target.getStr();
    if(url[0] != '/' || url[0] != 'h')
    {
        std::cerr << "Bad URL\n";
        return BAD_RESPONSE;
    }
    else if(url.find("..") != std::string::npos)
    {
        std::cerr << "Bad URL\n";
        return BAD_RESPONSE;
    }


    //construct a filepath
    //ensure the end of the path is a valid file




}