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
#include <boost/algorithm/string.hpp>
#include <filesystem>

using namespace boost;


namespace myHttpServer {
std::unordered_map<std::string, std::string> virtualHosts; 
std::string const acceptedFormats[] = {"text/html", "text/plain"};
std::string const acceptedFormatsEndings[] = {".html", ".txt"};
size_t const lengthAcceptedFormats = 2;
struct ContentSelection {
    int formatArray[lengthAcceptedFormats];
    int userAgent = -1; //1 is mobile 0 is computer
    std::chrono::system_clock::time_point ifModifiedSince;
    int connection; //0 is close, 1 is keep-alive 
    std::string username;
    std::string password;

       ContentSelection() : formatArray{} {} 
};

void fillInBadResponse(HttpResponse* response)
{
    return; 
}

namespace fs = std::__fs::filesystem;


bool isExecutable(const std::string& filePath) {
    try {
        // Use std::filesystem to check if the file exists and is executable
        fs::path path(filePath);
        return fs::exists(path) && fs::is_regular_file(path) && (fs::status(path).permissions() & fs::perms::owner_exec) == fs::perms::owner_exec;

    } catch (const std::exception& ex) {
        std::cerr << "Error checking file permissions: " << ex.what() << std::endl;
        return false;
    }
}

HttpResponse runExecutable(const std::string& filepath) {
    std::string command = filepath;
    std::string cgiOutput;

    // Execute the CGI script and capture its output
    FILE* cgiProcess = popen(command.c_str(), "r");
    if (cgiProcess) {
        char buffer[4096];
        while (fgets(buffer, sizeof(buffer), cgiProcess) != nullptr) {
            cgiOutput += buffer;
        }
        pclose(cgiProcess);

        // Return the CGI script's output as an HTTP response
        HttpResponse response(HttpStatusCode::Ok);
        response.SetContent(cgiOutput, filepath);
        return response;
    } else {
        // Handle error if the CGI script couldn't be executed
        return HttpResponse(HttpStatusCode::InternalServerError);
    }
}

//this method assumes the .htaccess file is perfectly formatted
int parseAuthFile(std::string dirToCheck, ContentSelection* contentCriteria) //parsing based on spec format
{
    //format of file:
    /*
        AuthType Basic
        AuthName "Restricted Files"
        User base64-encoded-user-name
        Password base64-encoded-password
    */
    std::string dataChunk; 
    std::string pass;
    std::string user;
    std::ifstream fileData(dirToCheck + "/.htaccess");
    while (std::getline (fileData, dataChunk)) {
        std::cout << "datachunk: " << dataChunk << "\n";
        if(dataChunk.find("User") != std::string::npos){
            user = dataChunk.substr(5, dataChunk.length() - 5 - 1);
        }
        else if(dataChunk.find("Password") != std::string::npos)
        {
            pass = dataChunk.substr(9, dataChunk.length() - 9 - 1);
        }
    }

    std::cout << "user " << user << "\n";
    std::cout << "pass " << pass << "\n";

    //decoding
    typedef boost::archive::iterators::transform_width<
    boost::archive::iterators::binary_from_base64<std::string::const_iterator>, 8, 6
    > base64_decode_iterator;

    base64_decode_iterator begin(user.begin());
    base64_decode_iterator end(user.end());
    std::string userDecoded(begin, end);

    std::cout << "User decoded:" << userDecoded << "\n";

    base64_decode_iterator passBegin(pass.begin());
    base64_decode_iterator passEnd(pass.end());
    std::string passDecoded(passBegin, passEnd);

    std::cout << "Pass decoded:" << passDecoded << "\n";
    
    std::cout << "Content pass decoded:" << contentCriteria->password << "\n";
    std::cout << "Content user decoded:" << contentCriteria->username << "\n";

    for(int i; i < passDecoded.length(); i++)
    {
        if (passDecoded[i] != contentCriteria->password[i])
        {
            std::cout << "not equal at " << i << " passdecoded " << int(passDecoded[i]) << " contentCriteria->password " << int(contentCriteria->password[i]) << "\n";
        }
    }
    if(contentCriteria->password == passDecoded && contentCriteria->username == userDecoded)
    {
        return 0;
    }
    else
    {
        return -1;
    }

}

int authorizationCheck(ContentSelection* contentCriteria, std::string filepath)
{
    std::string dirToCheck;
    bool authRequired = false;
    if(std::__fs::filesystem::is_directory(filepath))
    {
        dirToCheck = filepath;
    }
    else
    {
        std::__fs::filesystem::path myFilePath(filepath);
        dirToCheck = myFilePath.parent_path().c_str();
    }

    for (const auto& entry : std::__fs::filesystem::directory_iterator(dirToCheck)) 
    {
        if (entry.path().filename().compare(".htaccess") == 0)
        {
            authRequired = true;
        }
    }

    if(!authRequired)
    {
        return 0;
    }

    return parseAuthFile(dirToCheck, contentCriteria);

}

//returns the filepath of the resource to return
std::string contentSelection(const HttpRequest& request, HttpResponse* response, ContentSelection* contentCriteria, std::string filepath) {
    
    std::string indexPath;
    std::string mIndexPath;
    std::string toReturn = filepath;
    std::__fs::filesystem::path myFileObj(filepath);


    //check accept (assuming static file here)
    if(!request.headers()["Accept"].empty())
    {
        bool filepathValid = false;
        //see if filepath is valid
        if(std::__fs::filesystem::exists(filepath))
        {
            filepathValid = true;
        }

        std::cout << "here accept\n";
        std::cout << "html: " << contentCriteria->formatArray[0] << "text: " << contentCriteria->formatArray[1] << "\n";
        bool found = false;
        std::string alternative;
        for(int i = 0; i < lengthAcceptedFormats; i++)
        {
            std::cout << "i: " << i << "\n";
            if(filepathValid && contentCriteria->formatArray[i] == 1 && filepath.find(acceptedFormatsEndings[i]) != std::string::npos)
            {
                found = true;
                toReturn = filepath;
                break;
            }
            else if(contentCriteria->formatArray[i] == 1)
            {
                std::cout << "alternative here";
                std::cout << "empty val" << alternative.empty() << "\n";
                if(alternative.empty())
                {
                    if(filepath.find('.') != std::string::npos)
                    {
                        alternative = filepath.substr(0, filepath.find('.')) + acceptedFormatsEndings[i];
                    }
                    else
                    {
                        alternative = filepath + acceptedFormatsEndings[i];
                    }
                    std::cout << "alternative" << alternative;

                    if(!std::__fs::filesystem::exists(alternative))
                    {
                        alternative.clear();
                    }
                   
                }
            }
        }
        if(!found)
        {

            if (alternative != "")
            {
                toReturn = alternative;
            }
            else
            {
                return "NotFound";
            } 
        }
    }

    //first check authorization
    int ret = authorizationCheck(contentCriteria, filepath);
    if(ret != 0)
    {
        return "Unauthorized";
    }

    //if the path does not end in a file
    if(std::__fs::filesystem::is_directory(filepath))
    {
        //checking for index_m.html and index.html
        for (const auto& entry : std::__fs::filesystem::directory_iterator(filepath)) 
        {
            if (entry.is_regular_file() && entry.path().filename().compare("index_m.html") == 0) 
            {
                mIndexPath = filepath + "/index_m.html";
            }
            if (entry.is_regular_file() && entry.path().filename().compare("index.html") == 0) 
            {
                indexPath = filepath + "index.html";
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
        else if (!indexPath.empty()) //either non-mobile or don't know
        {
            toReturn = indexPath;
        }
        else
        {
            return "NotFound";
        }

    }

    
    //if the filepath exists, and the suffix is one of the data types the user supports, put it as the to return
    //if not, search for files in the dir that have that name with one of the supported suffixes 
    //if that fails return not found

    //check user agent
    std::cout << "userAgent " << contentCriteria->userAgent << "\n";
    std::__fs::filesystem::path myNewFileObj(toReturn);
    if(contentCriteria->userAgent != -1)
    {
        if(contentCriteria->userAgent == 1) //mobile
        {
            std::cout << "he's a mobile boi\n";
            if(myNewFileObj.filename().string().find("_m") == std::string::npos) //if they requested a non-mobile file
            {
                //look for a mobile version
                //constructing new name:
                std::string newName;
                size_t hasSuffix = myFileObj.filename().string().find('.');
                if(hasSuffix == std::string::npos)
                {
                    newName = myNewFileObj.filename().string() + "_m";
                }
                else
                {
                    newName = myNewFileObj.filename().string().substr(0, hasSuffix) + "_m" + myNewFileObj.filename().string().substr(hasSuffix, myFileObj.filename().string().length());
                }
                std::cout << "filename : " << myNewFileObj.filename() << "\n";
                std::cout << "newName: " << newName << "\n";

                boost::to_lower(newName);

                for (const auto& entry : std::__fs::filesystem::directory_iterator(myFileObj.parent_path())) 
                {
                    std::cout << "files " << entry.path().filename() << "\n";
                    std::string currentFile = entry.path().filename();
                    boost:to_lower(currentFile);

                    if (entry.is_regular_file() && currentFile.compare(newName) == 0) 
                    {
                        std::cout << "sucess!";
                        toReturn = myFileObj.remove_filename().string() + "/" + newName;
                    }

                }

            }
        }
        else
        {
            size_t filenameLen = myNewFileObj.filename().string().length();
            if(myNewFileObj.filename().string().find("_m.") != std::string::npos || (myNewFileObj.filename().string()[filenameLen - 1] == 'm' && myNewFileObj.filename().string()[filenameLen - 2] == '_')) //if they requested a non-mobile file
            {
                //look for a non - mobile version
                //constructing new name:
                std::string newName;
                size_t hasSuffix = myNewFileObj.filename().string().find('.');
                if(hasSuffix == std::string::npos)
                {
                    newName = myNewFileObj.filename().string().substr(0, filenameLen - 2);
                }
                else
                {
                    newName = myNewFileObj.filename().string().substr(0, hasSuffix - 2) + myNewFileObj.filename().string().substr(hasSuffix, myFileObj.filename().string().length());
                }

                boost::to_lower(newName);
              
                for (const auto& entry : std::__fs::filesystem::directory_iterator(myFileObj.parent_path())) 
                {
                    std::string currentFile = entry.path().filename();
                    boost::to_lower(currentFile);
                    if (entry.is_regular_file() && currentFile.compare(newName) == 0) 
                    {
                        toReturn = myFileObj.remove_filename().string() + "/" + newName;
                    }

                }

            }
        }
    }

    if(!request.headers()["If-Modified-Since"].empty()) //if there's an if modified since header, it's been parsed
    {
        // Use std::filesystem::last_write_time to get a std::filesystem::file_time_type
        std::cout << "recognizes header";
        std::__fs::filesystem::file_time_type fileTime = std::__fs::filesystem::last_write_time(toReturn);
        
        if(fileTime.time_since_epoch() < contentCriteria->ifModifiedSince.time_since_epoch())
        {
            
            return "Not modified:" + filepath;
        }
    } 

    return toReturn;

    //check ifmodifiedsince 
    //if has been modified, continue
    //if not been modified, return 304 not modified

}


int noContentRequired(std::string filepath, HttpResponse* ourResponse)
{
    if(filepath.find("Not modified") != std::string::npos)
    {
        std::cout << "not modified\n";
        std::string actual_filepath = filepath.substr(filepath.find(":") + 1);
        (*ourResponse).SetStatusCode(myHttpServer::HttpStatusCode::NotModified);
        (*ourResponse).SetLastModified(actual_filepath);

        return 1;
    }
    else if (filepath == "NotFound")
    {
        (*ourResponse).SetStatusCode(myHttpServer::HttpStatusCode::NotFound);
        return 1;
    }
    else if (filepath == "Unauthorized")
    {
        (*ourResponse).SetStatusCode(myHttpServer::HttpStatusCode::Unauthorized);
        return 1;
    }
    return 0;
}


HttpResponse RequestHandlers::GetHandler(const HttpRequest& request) 
{
    std::cout << "hello!!!\n";
    //for testing, need to implement virtualHosts
    ////virtualHosts.insert({"root", "/Users/samdetor/http_server/src"});
    //virtualHosts.insert({"root", "/Users/jennymao/Documents/repos/http_server/src"});
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
    if(virtualHosts.find(request.headers()["Host"]) != virtualHosts.end())
    {
        root = virtualHosts[request.headers()["Host"]];
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
    if(!request.headers()["User-Agent"].empty())//request.headers().find("UserAgent") != request.headers().end())
    {
        //right now we only handle mobile user or not
        std::string user_agent = request.headers()["User-Agent"];
        std::cout << "user_agent str: " << user_agent << "\n";
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
        if (authorization_string.substr(0, 5).compare("Basic") != 0)
        {
            std::cerr << "Unsupported authentification\n";
        }
        else
        {
            std::string userPass = authorization_string.substr(5, authorization_string.length() - 5 - 1);
            std::cout << "UserPass " << userPass << "\n";
            
            // Decoding the username::password
            typedef boost::archive::iterators::transform_width<
            boost::archive::iterators::binary_from_base64<std::string::const_iterator>, 8, 6
            > base64_decode_iterator;

            base64_decode_iterator begin(userPass.begin());
            base64_decode_iterator end(userPass.end());

            std::string decoded(begin, end);
            std::cout << "decoded " << decoded << "\n";
            size_t locationOfColon = decoded.find(":");
            std::string username = decoded.substr(0,locationOfColon);
            std::string password = decoded.substr(locationOfColon + 1, decoded.length());

            //add the two decoded values to the content generation method 
            contentSelectionCriteria.username = username;
            contentSelectionCriteria.password = password;
        }
    }

    //construct a filepath
    std::string filepath = root + our_uri;
    std::cout << "filepath: " << filepath << "\n";

   

    std::cout << "here1\n";

    // handle load balance query endpoint 
    if (our_uri == "/load")
    {
        int newRequests = 1000;
        if (newRequests < 0 || newRequests > 100000) {
            ourResponse.SetStatusCode(HttpStatusCode::ServiceUnavailable); //503
        }
        else  {
            ourResponse.SetStatusCode(HttpStatusCode::Ok); //200
        }

        // make wrapper function for headers
        return ourResponse; 
    }


    //select what file to send back based on headers
    filepath = contentSelection(request, &ourResponse, &contentSelectionCriteria, filepath); //will catch if filepath is invalid
    if(noContentRequired(filepath, &ourResponse))
    {
        return ourResponse;
    }
    std::cerr << "this file" << filepath << "\n";

    // check if file is CGI and executable 
    if (isExecutable(filepath)) {
        std::cerr << "it's executable " << filepath << "\n";
        return runExecutable(filepath);
    }

    //fill in response message
    //get the file contents into a buffer
    std::string data;
    std::string dataChunk;
    std::ifstream fileData(filepath);
    while (std::getline (fileData, dataChunk)) {
        data  = data + dataChunk + "\n";
    }
    std::cout << data << "\n";
    ourResponse.SetContent(data, filepath);
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

void RequestHandlers::RegisterGetHandlers(HttpServer& server) {
  // Define and register GET handlers here
    auto say_hello = [](const HttpRequest& request) -> HttpResponse {
        HttpResponse response(HttpStatusCode::Ok);
        response.SetHeader("Content-Type", "text/plain");
        response.SetContent("Hello, world\n", "/");
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
        response.SetContent(content, "/index.html");
    
        return response;
    };

    auto getHandler = [](const HttpRequest& request) -> HttpResponse {
        return RequestHandlers::GetHandler(request);
    };

    //server.RegisterHttpRequestHandler("/index.html", MethodType::GET, getHandler);
    server.RegisterHttpRequestHandler(MethodType::GET, getHandler);
    server.RegisterHttpRequestHandler("/", MethodType::GET, say_hello);
    //server.RegisterHttpRequestHandler("/hello.html", MethodType::GET, send_html);
}


void RequestHandlers::RegisterPostHandlers(HttpServer& server) {
    auto run_cgi = [] (const HttpRequest& request) -> HttpResponse {
        std::string postData = request.content(); // POST data as stdin for the CGI

        // Set up environment variables
        setenv("REQUEST_METHOD", "POST", 1);
        setenv("CONTENT_TYPE", "application/x-www-form-urlencoded", 1);
        setenv("CONTENT_LENGTH", std::to_string(postData.size()).c_str(), 1);

        std::string root;
        //validate URI
        std::string our_uri = request.uri().path();

        // configure virtual host 
        if(virtualHosts.find(request.uri().host()) != virtualHosts.end())
        {
            root = virtualHosts[request.uri().host()];
        }
        else
        {
            root = virtualHosts["root"]; //unspecified host leads to root virtual host being used
        }
        
        std::string filepath = root + our_uri;
        std::cout << "filepath: " << filepath << "\n";


        // Get the path to the CGI script for the requested URI
        //std::string cgiScriptPath = request.uri().path();
        // TOOD: need to replace this with the request path -- have to start at root
        std::string cgiScriptPath = "../cgi-bin/script_cgi.pl";

        // TODO: check if the cgiScriptPath is in the list of existing resources
        if(!std::__fs::filesystem::exists(filepath)) {
            std::cout << "not in path of resources" << "\n";
            // error 
            return HttpResponse(HttpStatusCode::InternalServerError);
        }

        // check if mapped file is executable 
        if (isExecutable(filepath)) {

            std::cout << "executable" << "\n";
            
            // execute the CGI script and capture its output
            std::string command = filepath;

            // use popen to execute the CGI script and read its output
            FILE* cgiProcess = popen(command.c_str(), "w");
            if (!cgiProcess) {
                // handle error if the CGI script couldn't be executed
                return HttpResponse(HttpStatusCode::InternalServerError);
            }

            // Write the POST data to the CGI process's stdin
            fwrite(postData.c_str(), 1, postData.length(), cgiProcess);
            fclose(cgiProcess);

            // Read and capture the output of the CGI script (stdout)
            std::string cgiOutput;
            char buffer[4096];
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
            response.SetContent(cgiOutput, filepath);
            return response;
        }
        else {
            // error handling if the mapped file is not executable 
            return HttpResponse(HttpStatusCode::InternalServerError);
        }

    };
    
    server.RegisterHttpRequestHandler(MethodType::POST, run_cgi);
}

void RequestHandlers::RegisterHandlers(HttpServer& server) {

    // Register GET and POST request handlers
    RegisterGetHandlers(server);
    RegisterPostHandlers(server);
    // Register all 50 request handlers here
}


void RequestHandlers::ParseConfigFile(std::string configfile, int* port, int* selectLoops)
{
  std::cout << "hello\n";
  std::string dataChunk;
  std::ifstream fileData(configfile);
  std::string DocumentRoot;
  std::string ServerName;
  while (std::getline (fileData, dataChunk)) { //parsing based on example config in spec, only supports listeing on one port
          std::cout << "DataChunk: " << dataChunk << "\n";
          if (dataChunk.find("Listen") != std::string::npos)
          {
            *port = std::stoi(dataChunk.substr(7, dataChunk.length()));
          }

          else if (dataChunk.find("nSelectLoops") != std::string::npos)
          {
            *selectLoops = std::stoi(dataChunk.substr(13, dataChunk.length()));
            std::cout << "chunk: " << dataChunk.substr(13, dataChunk.length()) << "\n";
          }

          else if (dataChunk.find("<VirtualHost") != std::string::npos){
            while (std::getline (fileData, dataChunk)) {
              if(dataChunk.find("DocumentRoot") != std::string::npos)
              {
                DocumentRoot = dataChunk.substr(dataChunk.find("DocumentRoot ") + 14), dataChunk.length();
              }
              
              else if(dataChunk.find("ServerName") != std::string::npos)
              {
                ServerName = dataChunk.substr(dataChunk.find("ServerName ") + 11, dataChunk.length());
              }
              
              else if(dataChunk.find("</VirtualHost>") != std::string::npos){
                virtualHosts[ServerName] = DocumentRoot;
                if(virtualHosts.size() == 1)
                {
                  virtualHosts["root"] = DocumentRoot;
                }
                break;
              }
          }

        }

  }
  std::cout << "end\n";
}

void RequestHandlers::PrintVirtualHosts(void)
{
    // Iterate over the elements and print them
    for (const auto& pair : virtualHosts) {
        std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
    }
}

}