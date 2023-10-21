#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> 
#include <arpa/inet.h>
#include "Message_new.h"

bool isVersion11(HTTPRequest* request);

std::string to_string(const HTTPRequest& request){
}

//currently assumes full request is available for parsing but is written so that it will be easily able to pivot 
//to being called by a non blocking handler
void string_to_request(const std::string& request_string, HTTPRequest* new_request) {
    new_request->raw_request = request_string;
    new_request->stringLen = request_string.length();
    size_t nextSpace;

    while(new_request->index < new_request->stringLen )
    {
        std::cout << new_request->index << "\n";
        switch(new_request->requestStage)
        {
            case ProcessingStage::METHOD:
                if (new_request->index == 0)
                {
                    if(new_request->raw_request[0] == 'G')
                    {
                        new_request->method = MethodType::GET;
                    }
                    else if (new_request->raw_request[0] == 'P')
                    {
                        new_request->method = MethodType::POST;
                    }
                }
                nextSpace = new_request->raw_request.find(' ');
                
                if (nextSpace != std::string::npos && nextSpace + 1 < new_request->stringLen)
                {
                    new_request->index = nextSpace + 1;
                    new_request->requestStage = ProcessingStage::TARGET;
                }
                else
                {
                    return;
                }
                break;

            case ProcessingStage::TARGET:
                nextSpace = new_request->raw_request.substr(new_request->index, new_request->stringLen).find(32); //+ new_request->index; //space char
                
                if(nextSpace != std::string::npos)
                {
                    
                    new_request->target.setTarget(new_request->raw_request.substr(new_request->index, nextSpace));
                    new_request->index = nextSpace + 1;
                    new_request->requestStage = ProcessingStage::VERSION;
                }
                
                if(new_request->index >= new_request->stringLen)
                {
                    return;
                }

                break;

            case ProcessingStage::VERSION:
                
                if(isVersion11(new_request))
                {
                    new_request->version = Version::HTTP_1_1;
                }


                size_t nextNewLine = new_request->raw_request.find("\r\n"); 

                if(nextNewLine != std::string::npos)
                {
                    new_request->index = nextNewLine + 1;
                    if(nextNewLine + 1 < new_request->stringLen)
                    {
                        new_request->requestStage = ProcessingStage::HOST;
                    }
                    else
                    {
                        return;
                    }
                }
                else
                {
                    return;
                }
            break;

            case ProcessingStage::HOST:
                size_t nextColon = new_request->raw_request.substr(new_request->index, new_request->stringLen).find(":");
                nextNewLine = new_request->raw_request.substr(nextColon, new_request->stringLen).find("\r\n");

                if(nextColon != std::string::npos && nextNewLine != std::string::npos)
                {
                    new_request->host.setTarget(new_request->raw_request.substr(nextColon + 1, nextNewLine));
                    new_request->index = nextNewLine + 1;
                }
                else
                {
                    return;
                }
            case ProcessingStage::HEADERS:
                //if there is a CRLF and a colon in a line, its a header
                //if just CRLF its message body
                //how to know when message is over??? idk yet
                nextColon = new_request->raw_request.substr(new_request->index, new_request->stringLen).find(":");
                nextNewLine = new_request->raw_request.substr(nextColon, new_request->stringLen).find("\r\n");

                if(nextColon != std::string::npos && nextNewLine != std::string::npos)
                {
                    whichHeader(new_request, new_request->raw_request.substr(new_request->index, nextColon), new_request->raw_request.substr(nextColon + 1, nextNewLine));
                }
                


        }
        
        
        
    }

    return;
    
}


bool isVersion11(HTTPRequest* request)
{
    std::string comparison = "HTTP/1.1";
    int comparisonLen = 8;
    int originalIndex = request->index;
    int currentIndex = request->index;
    int comparisionIndex = 0;
    int matches = 0;

    while(currentIndex < request->stringLen && comparisionIndex < comparisonLen)
    {
        if(comparison[comparisionIndex] != request->raw_request[currentIndex])
        {
            break;
        }
        else
        {
            matches++;
        }
        comparisionIndex++;
        currentIndex++;
    }

    if(matches < 8)
    {
        return true;
    }

    return false;
 
}

int whichHeader(HTTPRequest* request, std::string header, std::string value)
{
    //headers we accept rn
    //Accept
    //User-Agent
    //If-Modified-Since
    //Connection
    //Authorization

    //finding which header:
    if (header[0] == 'U')
    {
        parseUserAgent(request, header, value);
    }
    else if (header[0] = 'I')
    {
        parseIfModified(request, header, value);
    }
    else if (header[0] == 'C')
    {
        parseConnection(request, header, value);
    }


}

/* std::string to_string(const HttpResponse& response, bool send_content = true){

}
HttpResponse string_to_response(const std::string& response_string){
} */