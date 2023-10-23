#include "Message_new.h"

bool isVersion11(HTTPRequest* request);
int whichHeader(HTTPRequest* request, std::string header, std::string value);

std::string to_string(const HTTPRequest& request){
}

//currently assumes full request is available for parsing but is written so that it will be easily able to pivot 
//to being called by a non blocking handler
int string_to_request(const std::string& request_string, HTTPRequest* new_request) {
    new_request->raw_request = request_string;
    new_request->stringLen = request_string.length();
    size_t nextSpace;
    size_t nextColon;
    size_t nextNewLine;

    while(new_request->index < new_request->stringLen )
    {
       // std::cout << new_request->index << "\n";
        switch(new_request->requestStage)
        {
            case ProcessingStage::METHOD:
                
                if (new_request->index == 0)
                {
                    if(new_request->raw_request[0] == 'G')
                    {
                        std::cout << "Method GET\n";
                        new_request->method = MethodType::GET;
                    }
                    else if (new_request->raw_request[0] == 'P')
                    {
                        std::cout << "Method POST\n";
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
                    return 1;
                }
                break;

            case ProcessingStage::TARGET:
                nextSpace = new_request->raw_request.substr(new_request->index, new_request->stringLen).find(32); //+ new_request->index; //space char
                
                if(nextSpace != std::string::npos)
                {
                    std::cout << "target: " << new_request->raw_request.substr(new_request->index, nextSpace) << "\n";
                    new_request->target.setTarget(new_request->raw_request.substr(new_request->index, nextSpace));
                    new_request->index = nextSpace + 1;
                    new_request->requestStage = ProcessingStage::VERSION;
                }
                
                if(new_request->index >= new_request->stringLen)
                {
                    return 1;
                }

                break;

            case ProcessingStage::VERSION:
                
                if(isVersion11(new_request))
                {
                    std::cout << "version 1.1 \n";
                    new_request->version = Version::HTTP_1_1;
                }


                nextNewLine = new_request->raw_request.find("\r\n"); 

                if(nextNewLine != std::string::npos)
                {
                    new_request->index = nextNewLine + 1;
                    //std::cout << "substring after version: " << new_request->raw_request.substr(nextNewLine + 1, new_request->stringLen) << "\n";
                    if(nextNewLine + 1 < new_request->stringLen)
                    {
                        new_request->requestStage = ProcessingStage::HOST;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else
                {
                    return 1;
                }
            break;

            case ProcessingStage::HOST:
                //std::cout << "host\n";
                nextColon = new_request->raw_request.substr(new_request->index, new_request->stringLen).find(":") + new_request->index;
                nextNewLine = new_request->raw_request.substr(nextColon, new_request->stringLen).find("\r\n");
                //std::cout << "nextColon " << nextColon << "\n";
                //std::cout << "substring after nextColon " << new_request->raw_request.substr(nextColon, new_request->stringLen) << "\n";
                //std::cout << "nextNewLine " << nextNewLine << "\n";
                if(nextColon != std::string::npos && nextNewLine != std::string::npos)
                {
                    std::cout << "host " << new_request->raw_request.substr(nextColon + 2, nextNewLine - 1) << "\n";
                    std::cout << "newIndex: " << nextNewLine + 1 + nextColon << "\n";
                    new_request->host.setTarget(new_request->raw_request.substr(nextColon + 2, nextNewLine - 1));
                    new_request->index = nextNewLine + 2 + nextColon;
                    new_request->requestStage = ProcessingStage::HEADERS;
                }
                else
                {
                    return 1;
                }
            case ProcessingStage::HEADERS:
                //if there is a CRLF and a colon in a line, its a header
                //if just CRLF its message body
                //how to know when message is over??? idk yet
                //std::cout << "headers\n";
                nextColon = new_request->raw_request.substr(new_request->index, new_request->stringLen).find(":");
                nextNewLine = new_request->raw_request.substr(nextColon + new_request->index, new_request->stringLen).find("\r\n");
                //std::cout << "nextColon "<< nextColon <<"\n";
                //std::cout << "nextNewline "<< nextColon <<"\n";
                //std::cout << "index "<< new_request->index <<"\n";
                //if it was a header line
                if(nextColon != std::string::npos && nextNewLine != std::string::npos && nextColon + new_request->index < nextNewLine + nextColon + new_request->index)
                {
                    std::cout << "here\n";
                    new_request->headers.insert({new_request->raw_request.substr(new_request->index, nextColon), new_request->raw_request.substr(nextColon + 1 + new_request->index, nextNewLine)});
                    //whichHeader(new_request, new_request->raw_request.substr(new_request->index, nextColon), new_request->raw_request.substr(nextColon + 1 + new_request->index, nextNewLine));
                    new_request->index = nextNewLine + 2 + new_request->index + nextColon;
                }
                else if(nextNewLine != std::string::npos)
                {
                    // if (nextNewLine + new_request->index + 2 < new_request->stringLen)
                    // {
                    //     //std::cout << "at 1 " << uint8_t (new_request->raw_request[nextNewLine + 1]) << "\n";
                    //     std::cout << "substring after index: " << new_request->raw_request.substr(new_request->index, new_request->stringLen) << "\n";
                    //     if(new_request->raw_request[nextNewLine + new_request->index + 1] == '\r' && new_request->raw_request[nextNewLine + new_request->index + 2] == '\n') //detecting the CRLF after the headers line
                    //     {
                            
                            if (new_request->method == MethodType::GET) //gets not supposed to have a request body
                            {
                                return 0; //all done
                            }
                            else if (new_request->method == MethodType::POST)
                            {
                                new_request->requestStage = ProcessingStage::BODY;
                                new_request->index = nextNewLine + 1 + new_request->index + nextColon;
                            }
                            else
                            {
                                std::cerr << "Unrecognized request\n";
                                return -1;
                            }
                        //}
                    //}
                }
                else
                {
                    return 1;
                }
            break;

            case ProcessingStage::BODY: //for post, needs content length header to be parsed probs
                std::cerr << "Not implemented yet\n";
            break;
                
        }
        
    }

    return 1;
    
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

/* int whichHeader(HTTPRequest* request, std::string header, std::string value)
{
    //headers we accept rn
    //Accept
    //User-Agent
    //If-Modified-Since
    //Connection
    //Authorization

    //finding which header:
    if (header[0] == 'U') //user agent
    { 
        std::cout << "user agent\n";  
        request->headers.user_agent.parse(value);
    }
    else if (header[0] == 'I') //if modified since
    {
        std::cout << "if modified since\n";
        request->headers.modified.parse(value);
    }
    else if (header[0] == 'C') //connection
    {
        std::cout << "connection\n";
        request->headers.connection.parse(value);
    }
    else if (header[0] == 'A')
    {
        if(header[1] == 'c') //accept
        {
            std::cout << "Accept\n";
            std::cout << "value: " << value << "\n\n";
            request->headers.accept.parse(value);
        }
        else if (header[1] == 'u')
        {
            std::cout << "authorization\n";
            request->headers.authorization.parse(value);
        }
    }
    else
    {
        std::cerr << "Unrecognized header\n";
        std::cout << "Header " << header << "\n\n" <<  "Value: " << value << "\n\n";  
    }
    return 0;
} */

/* std::string to_string(const HttpResponse& response, bool send_content = true){

}
HttpResponse string_to_response(const std::string& response_string){
} */