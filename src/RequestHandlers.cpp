// RequestHandlerRegistry.cpp
#include "RequestHandlers.h"
#include "Server.h"
#include "Message.h"
#include "Uri.h"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <unordered_map>
#include <fstream>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <filesystem>

using namespace boost;

namespace simple_http_server {

std::unordered_map<std::string, std::string> virtualHosts;
std::string const acceptedFormats[] = {"text/html", "text/plain"};
size_t const lengthAcceptedFormats = 2;
struct ContentSelection {
    int formatArray[lengthAcceptedFormats];
    int userAgent; //1 is mobile 0 is computer
    std::chrono::system_clock::time_point ifModifiedSince;
    int connection; //0 is close, 1 is keep-alive 
    std::string username;
    std::string password;

        ContentSelection() : formatArray{} {} 
};


HttpResponse SayHelloHandler(const HttpRequest& request) {
    HttpResponse response(HttpStatusCode::Ok);
    response.SetHeader("Content-Type", "text/plain");
    response.SetContent("Hello, world\n");
    return response;
}

void fillInBadResponse(HttpResponse* response)
{
    return;
}

//returns the filepath of the resource to return
std::string contentSelection(const HttpRequest& request, HttpResponse* response, ContentSelection* contentCriteria, std::string filepath) {
    
    std::string indexPath;
    std::string mIndexPath;
    std::string toReturn;
    //if the path does not end in a file
    if(std::__fs::filesystem::is_directory(filepath))
    {
        //checking for index_m.html and index.html
        for (const auto& entry : std::__fs::filesystem::directory_iterator(filepath)) 
        {
            if (entry.is_regular_file() && entry.path().filename() == "index_m.html") 
            {
                mIndexPath = filepath + "/index_m.html";
            }
            if (entry.is_regular_file() && entry.path().filename() == "index.html") 
            {
                indexPath = filepath + "/index_m.html";
            }

        }
        //if mobile agent
        if(contentCriteria->userAgent == 1)
        {
            if(!mIndexPath.empty())
            {
                toReturn =  mIndexPath;
            }

        }
        else if (!indexPath.empty())
        {
            toReturn = indexPath;
        }
        else
        {
            return "NotFound";
        }

    }

    //check accept
    //if the filepath exists, and the suffix is one of the data types the user supports, put it as the to return
    //if not, search for files in the dir that have that name with one of the supported suffixes 
    //if that fails return not found

    //check user agent
    //if it is not mobile, and requested something with a _m, give it the normal file
    //if it is mobile, check if the requested file has _m. in the name, if it does, do nothing
        //if it doesn't, search for files with the requested file name _m with one of the accepted suffixes
            //if not found return not found response

    //check ifmodifiedsince 
    //if has been modified, continue
    //if not been modified, return 304 not modified

    //authorization
    //if hacess file exists in dir
        //de-encode the username password associated with it and compare to content selection array values
        //if it fails
            //return 401 unauthorized
        //if it is good
            //return filepath, all done
    //if doesn't exist
        //return filepath, all done





}



HttpResponse GetHandler(const HttpRequest& request) 
{
    std::cout << "hello!!!\n";
    //for testing, need to implement virtualHosts
    virtualHosts.insert({"root", "/Users/samdetor/http_server/src"});
    
    HttpResponse ourResponse;
    ContentSelection contentSelectionCriteria;
    std::string root;
    //validate URI
    std::string our_uri = request.uri().path();
     if(our_uri[0] != '/' && our_uri[0] != 'h')
    {
        std::cerr << "Bad URL1: " << our_uri[0] << "\n";
        fillInBadResponse(&ourResponse);
        return ourResponse;
    }
    else if(our_uri.find("..") != std::string::npos)
    {
        std::cerr << "Bad URL2:" << our_uri << "\n";
        fillInBadResponse(&ourResponse);
        return ourResponse;
    }

    //Virtual Host
    std::cout << "host: " << request.uri().host() << "\n";
    if(virtualHosts.find(request.uri().host()) != virtualHosts.end())
    {
        root = virtualHosts[request.uri().host()];
    }
    else
    {
        root = virtualHosts["root"]; //unspecified host leads to root virtual host being used
    }

    //Accept Header
    if(!request.headers()["Accept"].empty()) //request.headers().find("Accept") != request.headers().end())
    {
        //parse accept, we only handle text/html, text/plain for now
        std::string values = request.headers()["Accept"];
        std::stringstream ss(values);

        std::vector<std::string> tokens;
        std::string token;

        while (std::getline(ss, token, ',')) {
            tokens.push_back(token); //tokens has all of the accepted formats
        }

        for (const std::string& token : tokens) {
           if(token.compare(acceptedFormats[0]) == 0) //could eventually turn this into a for
           {
                //add HTML to some sort of content generation array
                contentSelectionCriteria.formatArray[0] = 1;
                
           }
           if (token.compare(acceptedFormats[1]) == 0)
           {
               //add plain to some sort of content generation array
               contentSelectionCriteria.formatArray[1] = 1;
           }
           //if its not one of the type we handle, ignore
        }

    }

    //User-Agent
    if(!request.headers()["UserAgent"].empty())//request.headers().find("UserAgent") != request.headers().end())
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
            contentSelectionCriteria.userAgent = 1;
        }
        else
        {
            //add not mobile to some sort of content generation array
            contentSelectionCriteria.userAgent = 0;
        }

    }
    //broken rn
    if(!request.headers()["If-Modified-Since"].empty())
    {
        //parsing the Http time/date format
        std::istringstream ss(request.headers()["If-Modified-Since"]);
        std::tm timeInfo = {};
        ss.imbue(std::locale("C")); // Set locale to "C" for consistent parsing

        ss >> std::get_time(&timeInfo, "%a, %d %b %Y %H:%M:%S GMT");
        if (ss.fail()) {
            std::cerr << "Weird Thing happening w/ time\n";
        }

        std::time_t time = std::mktime(&timeInfo);
        std::chrono::system_clock::time_point compareable_time = std::chrono::system_clock::from_time_t(time);

        //add comparable time object to content gen data structure
        contentSelectionCriteria.ifModifiedSince = compareable_time;

    }
     if(!request.headers()["Connection"].empty()) //request.headers().find("Connection") != request.headers().end())
    {
        std::vector<std::string> connectionAcceptedValues = {"close", "keep-alive"};
        if(request.headers()["Connection"].compare(connectionAcceptedValues[0]))
        {
            //set some overarching connection var, this can't be just achieved in the response
            contentSelectionCriteria.connection = 0;
        }
        else if (request.headers()["Connection"].compare(connectionAcceptedValues[1]))
        {
            //set some overarching connection var also
            contentSelectionCriteria.connection = 1;
        }

    }
    if(!request.headers()["Authorization"].empty())
    {
        //check that it is the basic form
        std::string authorization_string = request.headers()["Authorization"];
        std::cout << "Auth: " << request.headers()["Authorization"] << "\n";
        if (authorization_string.substr(0, 6).compare("Basic ") != 0)
        {
            std::cerr << "Unsupported authentification\n";
        }
        std::string userPass = authorization_string.substr(6, authorization_string.length());
        
        // Decoding the username::password
        typedef boost::archive::iterators::transform_width<
        boost::archive::iterators::binary_from_base64<std::string::const_iterator>, 8, 6
        > base64_decode_iterator;

        base64_decode_iterator begin(userPass.begin());
        base64_decode_iterator end(userPass.end());

        std::string decoded(begin, end);
        size_t locationOfColon = decoded.find(":");
        std::string username = decoded.substr(0,locationOfColon);
        std::string password = decoded.substr(locationOfColon + 1, decoded.length());

        //add the two decoded values to the content generation method 
        contentSelectionCriteria.username = username;
        contentSelectionCriteria.password = password;
    }

    //construct a filepath
    std::string filepath = root + our_uri;
    std::cout << "filepath: " << filepath << "\n";
    //ensure the path is vald
    if(std::__fs::filesystem::exists(filepath)) 
    {
        std::cout << "here1\n";
        //select what file to send back based on headers
        //filepath = contentSelection(request, &ourResponse, &contentSelectionCriteria, filepath);
        //fill in response message
        //get the file contents into a buffer
        std::string data;
        std::string dataChunk;
        std::ifstream fileData(filepath);
        while (std::getline (fileData, dataChunk)) {
            data  = data + dataChunk + "\n";
        }
        std::cout << data << "\n";
        ourResponse.SetContent(data);
        ourResponse.SetStatusCode(HttpStatusCode::Ok);
        std::cout << filepath << "\n";
        if(filepath.substr(filepath.length() - 5) == ".html")
        {
            std::cout << "html" << "\n";
            ourResponse.SetHeader("Content-Type", "text/html");
        }
        else
        {
            ourResponse.SetHeader("Content-Type", "text/plain");
        }
        return ourResponse;
       
    }
    else
    {
        std::cerr << "Bad URL (filepath): " << filepath << "\n";
        fillInBadResponse(&ourResponse);
        return ourResponse;
    }


    }

void RequestHandlers::RegisterGetHandlers(HttpServer& server) {
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

    auto getHandler = [](const HttpRequest& request) -> HttpResponse {
        return GetHandler(request);

    };

    //server.RegisterHttpRequestHandler("/index.html", HttpMethod::GET, getHandler);
    server.RegisterHttpRequestHandler(HttpMethod::GET, getHandler);
    //server.RegisterHttpRequestHandler("/hello.html", HttpMethod::GET, send_html);
}


void RequestHandlers::RegisterPostHandlers(HttpServer& server) {

    // 
    auto run_cgi = [] (const HttpRequest& request) -> HttpResponse {
        std::string postData = request.content(); // POST data as stdin for the CGI

        // Set up environment variables
        setenv("REQUEST_METHOD", "POST", 1);
        setenv("CONTENT_TYPE", "application/x-www-form-urlencoded", 1);
        setenv("CONTENT_LENGTH", std::to_string(postData.size()).c_str(), 1);


        // Get the path to the CGI script for the requested URI
        //std::string cgiScriptPath = request.uri().path();
        // TOOD: need to replace this with the request path -- have to start at root
        std::string cgiScriptPath = "../cgi-bin/script_cgi.pl";

        // Execute the CGI script and capture its output
        std::string command = cgiScriptPath;

        // Use popen to execute the CGI script and read its output
        FILE* cgiProcess = popen(command.c_str(), "w");
        if (!cgiProcess) {
            // Handle error if the CGI script couldn't be executed
            return HttpResponse(HttpStatusCode::InternalServerError);
        }

        // Write the POST data to the CGI process's stdin
        fwrite(postData.c_str(), 1, postData.length(), cgiProcess);
        fclose(cgiProcess);

        // Read and capture the output of the CGI script (stdout)
        std::string cgiOutput;
        char buffer[1024];
        cgiProcess = popen(command.c_str(), "r");
        if (!cgiProcess) {
            // Handle error if reading CGI output failed
            return HttpResponse(HttpStatusCode::InternalServerError);
        }
        while (fgets(buffer, sizeof(buffer), cgiProcess) != nullptr) {
            cgiOutput += buffer;
        }
        fclose(cgiProcess);

        // Construct an HTTP response with the CGI output
        HttpResponse response(HttpStatusCode::Ok);
        response.SetHeader("Content-Type", "text/plain"); // Set appropriate content type
        response.SetContent(cgiOutput);

        return response;
    };
    
    server.RegisterHttpRequestHandler("/cgi-bin/script_cgi.pl", HttpMethod::POST, run_cgi);
    //server.RegisterHttpRequestHandler("/hello.html", HttpMethod::GET, send_html);
}

void RequestHandlers::RegisterHandlers(HttpServer& server) {

    // Register GET and POST request handlers
    RegisterGetHandlers(server);
    RegisterPostHandlers(server);
    // Register all 50 request handlers here
}

}
