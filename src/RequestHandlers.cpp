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
#include <sys/stat.h>

using namespace boost;
namespace fs = std::__fs::filesystem;

namespace myHttpServer {
std::unordered_map<std::string, std::string> virtualHosts; 
std::string const acceptedFormats[] = {"text/html", "text/plain", "*/*"};
std::string const acceptedFormatsEndings[] = {".html", ".txt", ".pl"};
size_t const lengthAcceptedFormats = 3;
struct ContentSelection {
    int formatArray[lengthAcceptedFormats];
    int userAgent = -1; //1 is mobile 0 is computer
    std::time_t ifModifiedSince;
    int connection; //0 is close, 1 is keep-alive 
    std::string username;
    std::string password;

       ContentSelection() : formatArray{} {} 
};


////////////////////////////////////////////////////////////
// Config file parsing and virtual hosts
////////////////////////////////////////////////////////////

void RequestHandlers::ParseConfigFile(std::string configfile, int* port, int* selectLoops) {
  std::string dataChunk;
  std::ifstream fileData(configfile);
  std::string DocumentRoot;
  std::string ServerName;
  while (std::getline (fileData, dataChunk)) { //parsing based on example config in spec, only supports listeing on one port
    std::cout << "DataChunk: " << dataChunk << "\n";
    if (dataChunk.find("Listen") != std::string::npos) {
        *port = std::stoi(dataChunk.substr(7, dataChunk.length()));
    }
    else if (dataChunk.find("nSelectLoops") != std::string::npos) {
        *selectLoops = std::stoi(dataChunk.substr(13, dataChunk.length()));
        std::cout << "chunk: " << dataChunk.substr(13, dataChunk.length()) << "\n";
    }
    else if (dataChunk.find("<VirtualHost") != std::string::npos) {
        while (std::getline (fileData, dataChunk)) { 
            if(dataChunk.find("DocumentRoot") != std::string::npos) {
                DocumentRoot = dataChunk.substr(dataChunk.find("DocumentRoot ") + 14), dataChunk.length();
            } 
            else if(dataChunk.find("ServerName") != std::string::npos) {
                ServerName = dataChunk.substr(dataChunk.find("ServerName ") + 11, dataChunk.length());
            }
            else if(dataChunk.find("</VirtualHost>") != std::string::npos) {
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
}

void RequestHandlers::PrintVirtualHosts(void) {
    // Iterate over the elements and print them
    for (const auto& pair : virtualHosts) {
        std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
    }
}





////////////////////////////////////////////////////////////
// Executable CGI Script Handling
////////////////////////////////////////////////////////////
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

HttpResponse runGetExecutable(const std::string& filepath) {
    std::string command = filepath;

    // Set up environment variables
    setenv("REQUEST_METHOD", "GET", 1);
    setenv("QUERY_STRING", filepath.c_str(), 1);
    setenv("REMOTE_ADDR", "127.0.0.1", 1); 
    setenv("SERVER_NAME", "localhost", 1); 
    setenv("SERVER_PORT", "8080", 1); 
    setenv("SERVER_SOFTWARE", "HTTP/1.1", 1); 

    // Execute the CGI script and capture its output
    FILE* cgiProcess = popen(command.c_str(), "r");

    if (cgiProcess) {
        std::string cgiOutput;
        char buffer[4096];

        while (fgets(buffer, sizeof(buffer), cgiProcess) != NULL) {
            cgiOutput += buffer; 
        }
    
        int status = pclose(cgiProcess);

        // Return the CGI script's output as an HTTP response
        HttpResponse response(HttpStatusCode::Ok);
        response.SetContent(cgiOutput, filepath);
        return response;
    } else {
        // Handle error if the CGI script couldn't be executed
        return HttpResponse(HttpStatusCode::InternalServerError);
    }
}

HttpResponse runPostExecutable(const std::string& filepath, const std::string& input) {
    
    std::string command = filepath;

    // Set up environment variables
    setenv("REQUEST_METHOD", "POST", 1);
    setenv("CONTENT_TYPE", "application/x-www-form-urlencoded", 1);
    setenv("CONTENT_LENGTH", std::to_string(input.size()).c_str(), 1);
    setenv("QUERY_STRING", command.c_str(), 1);
    setenv("REMOTE_ADDR", "127.0.0.1", 1); 
    setenv("SERVER_NAME", "localhost", 1); 
    setenv("SERVER_PORT", "6789", 1); 
    setenv("SERVER_SOFTWARE", "HTTP/1.1", 1); 

    int inputPipe[2];
    int outputPipe[2];

    if (pipe(inputPipe) == -1 || pipe(outputPipe) == -1) {
        std::cerr << "Pipe creation failed" << std::endl;
        return HttpResponse(HttpStatusCode::InternalServerError);
    }

    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "Fork failed and CGI script was not executed" << std::endl;
        // Handle error if the CGI script couldn't be executed
        return HttpResponse(HttpStatusCode::InternalServerError);

    } 
    else if (pid == 0) { // Child process
        close(inputPipe[1]); // Close write end of input pipe
        close(outputPipe[0]); // Close read end of output pipe

        // Redirect stdin to input pipe
        dup2(inputPipe[0], STDIN_FILENO);
        close(inputPipe[0]);

        // Redirect stdout to output pipe
        dup2(outputPipe[1], STDOUT_FILENO);
        close(outputPipe[1]);

        // Replace the child process with the desired command
        execl(command.c_str(), NULL);
        std::cerr << "Exec failed" << std::endl;
        return HttpResponse(HttpStatusCode::InternalServerError);

    } 
    else { // Parent process
        close(inputPipe[0]); // Close read end of input pipe
        close(outputPipe[1]); // Close write end of output pipe

        // Write input to the child process (stdin)
        write(inputPipe[1], input.c_str(), input.size());
        close(inputPipe[1]);

        // Read output from the child process (stdout)
        char buffer[kMaxBufferSize];
        //std::string output;
        std::vector<char> output;
        ssize_t bytesRead;

        while ((bytesRead = read(outputPipe[0], buffer, sizeof(buffer))) > 0) {
            //output.append(buffer, bytesRead);
            output.insert(output.end(), buffer, buffer + bytesRead);
        }
        close(outputPipe[0]);

        // Return the CGI script's output as an HTTP response
        HttpResponse response(HttpStatusCode::Ok);
        response.SetContent(output, output.size(), filepath);
        return response;
    }
}





////////////////////////////////////////////////////////////
// Authorization 
////////////////////////////////////////////////////////////

//this method assumes the .htaccess file is perfectly formatted
std::string parseAuthFile(std::string dirToCheck, ContentSelection* contentCriteria) //parsing based on spec format
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
    std::string authType;
    std::string authName;
    std::ifstream fileData(dirToCheck + "/.htaccess");
    while (std::getline (fileData, dataChunk)) {
        //std::cout << "datachunk: " << dataChunk << "\n";
        if(dataChunk.find("User") != std::string::npos){
            user = dataChunk.substr(5, dataChunk.length() - 5 - 1);
        }
        else if(dataChunk.find("Password") != std::string::npos)
        {
            pass = dataChunk.substr(9, dataChunk.length() - 9 - 1);
        }
        else if(dataChunk.find("AuthType")!= std::string::npos)
        {
            authType = dataChunk.substr(dataChunk.find("AuthType") + 9);
        }
        else if(dataChunk.find("AuthName")!= std::string::npos)
        {
            authName = dataChunk.substr(dataChunk.find("AuthName") + 9);
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
        return "ok";
    }
    else
    {
        std::cout << authType << ":" << authName << "\n";
        return authType + ":" + authName;
    }

}

std::string authorizationCheck(ContentSelection* contentCriteria, std::string filepath)
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
        return "ok";
    }

    return parseAuthFile(dirToCheck, contentCriteria);

}





////////////////////////////////////////////////////////////
// Content Selection 
////////////////////////////////////////////////////////////

//returns the filepath of the resource to return
std::string contentSelection(const HttpRequest& request, HttpResponse* response, ContentSelection* contentCriteria, std::string filepath) {
    
    std::string indexPath;
    std::string mIndexPath;
    std::string toReturn = filepath;
    std::__fs::filesystem::path myFileObj(filepath);

    //if the path does not end in a file
    if(std::__fs::filesystem::is_directory(filepath)) {
        //checking for index_m.html and index.html
        for (const auto& entry : std::__fs::filesystem::directory_iterator(filepath))  {
            if (entry.is_regular_file() && entry.path().filename().compare("index_m.html") == 0) {
                mIndexPath = filepath + "/index_m.html";
            }
            if (entry.is_regular_file() && entry.path().filename().compare("index.html") == 0) {
                indexPath = filepath + "index.html";
            }

        }
        //if mobile agent
        if(contentCriteria->userAgent == 1) {
            if(!mIndexPath.empty()) {
                toReturn =  mIndexPath;
            }

        }
        else if (!indexPath.empty()) { //either non-mobile or don't know
            toReturn = indexPath;
        }
        else {
            return "NotFound";
        }

    }

    //check accept (assuming static file here)
    //if the filepath exists, and the suffix is one of the data types the user supports, put it as the to return
    //if not, search for files in the dir that have that name with one of the supported suffixes 
    //if that fails return not found
    if(!request.headers()["Accept"].empty()) {
        bool filepathValid = false;
        //see if filepath is valid
        if(std::__fs::filesystem::exists(toReturn)) {
            filepathValid = true;
            std::cout << "Accept Header: filepath valid\n";
        }

        std::cout << "filepath " << toReturn << "\n";

        bool found = false;
        std::string alternative;

        std::cout << "Wildcard Val:" << contentCriteria->formatArray[2] << "\n";
        if (filepathValid && contentCriteria->formatArray[2] != 1) {
            for (int i = 0; i < lengthAcceptedFormats; i++) {
                //std::cout << "i: " << i << "\n";
                //std::cout << "content criteria " << contentCriteria->formatArray[i] << "\n";
                //std::cout << "accepted formats endings " << toReturn.find(acceptedFormatsEndings[i]) << "\n";
                if(filepathValid && contentCriteria->formatArray[i] == 1 && toReturn.find(acceptedFormatsEndings[i]) != std::string::npos) {
                    found = true;
                    break;
                }
                else if (contentCriteria->formatArray[i] == 1) {
                    //std::cout << "alternative here";
                    //std::cout << "empty val" << alternative.empty() << "\n";
                    if (alternative.empty()) {
                        if(toReturn.find('.') != std::string::npos) {
                            alternative = toReturn.substr(0, toReturn.find('.')) + acceptedFormatsEndings[i];
                        }
                        else {
                            alternative = toReturn + acceptedFormatsEndings[i];
                        }
                        //std::cout << "alternative" << alternative;

                        if(!std::__fs::filesystem::exists(alternative)) {
                            alternative.clear();
                        }
                    }
                }
            }
            if (!found) {
                if (alternative != "") {
                    toReturn = alternative;
                }
                else {
                    return "NotFound";
                } 
            }
        }
        else if (!filepathValid) {
            return "NotFound";
        }
    }

    //check user agent
    //std::cout << "userAgent " << contentCriteria->userAgent << "\n";
    std::__fs::filesystem::path myNewFileObj(toReturn);
    if(contentCriteria->userAgent != -1) {
        if(contentCriteria->userAgent == 1) { //mobile
            //std::cout << "he's a mobile boi\n";
            if(myNewFileObj.filename().string().find("_m") == std::string::npos) { //if they requested a non-mobile file
                //look for a mobile version
                //constructing new name:
                std::string newName;
                size_t hasSuffix = myFileObj.filename().string().find('.');
                if(hasSuffix == std::string::npos) {
                    newName = myNewFileObj.filename().string() + "_m";
                }
                else {
                    newName = myNewFileObj.filename().string().substr(0, hasSuffix) + "_m" + myNewFileObj.filename().string().substr(hasSuffix, myFileObj.filename().string().length());
                }
                //std::cout << "filename : " << myNewFileObj.filename() << "\n";
                //std::cout << "newName: " << newName << "\n";

                boost::to_lower(newName);

                for (const auto& entry : std::__fs::filesystem::directory_iterator(myFileObj.parent_path())){
                    //std::cout << "files " << entry.path().filename() << "\n";
                    std::string currentFile = entry.path().filename();
                    boost:to_lower(currentFile);

                    if (entry.is_regular_file() && currentFile.compare(newName) == 0) {
                        //std::cout << "sucess!";
                        toReturn = myFileObj.remove_filename().string() + "/" + newName;
                    }

                }

            }
        }
        else {
            size_t filenameLen = myNewFileObj.filename().string().length();
            if (myNewFileObj.filename().string().find("_m.") != std::string::npos || (myNewFileObj.filename().string()[filenameLen - 1] == 'm' && myNewFileObj.filename().string()[filenameLen - 2] == '_')) { //if they requested a non-mobile file
                //look for a non - mobile version
                //constructing new name:
                std::string newName;
                size_t hasSuffix = myNewFileObj.filename().string().find('.');
                if (hasSuffix == std::string::npos) {
                    newName = myNewFileObj.filename().string().substr(0, filenameLen - 2);
                }
                else {
                    newName = myNewFileObj.filename().string().substr(0, hasSuffix - 2) + myNewFileObj.filename().string().substr(hasSuffix, myFileObj.filename().string().length());
                }

                boost::to_lower(newName);
              
                for (const auto& entry : std::__fs::filesystem::directory_iterator(myFileObj.parent_path())) {
                    std::string currentFile = entry.path().filename();
                    boost::to_lower(currentFile);
                    if (entry.is_regular_file() && currentFile.compare(newName) == 0) {
                        toReturn = myFileObj.remove_filename().string() + "/" + newName;
                    }

                }

            }
        }
    }
    
    //check ifmodifiedsince 
    //if has been modified, continue
    //if not been modified, return 304 not modified
    if(!request.headers()["If-Modified-Since"].empty()) { //if there's an if modified since header, it's been parsed
        // Use std::filesystem::last_write_time to get a std::filesystem::file_time_type
        //std::cout << "recognizes header";
       
        //std::__fs::filesystem::file_time_type fileTime = std::__fs::filesystem::last_write_time(toReturn);
        
        struct stat fileStat;
        if (stat(toReturn.c_str(), &fileStat) != 0) {
            std::cerr << "File not found: " << toReturn << std::endl;
            return "Not found";
        }

        std::tm lastModifiedTM = *std::gmtime(&fileStat.st_mtime);
        std::time_t timeSinceEpoch = std::mktime(&lastModifiedTM);
        
        //std::chrono::system_clock::time_point timePoint = std::chrono::system_clock::from_time_t(fileStat.st_mtime);

       //std::time_t lastModified = fileStat.st_mtime;
        
        //std::cout << "Last Modified: " << fileTime.to_string() << "\n";
        std::string timeString1 = std::ctime(&(timeSinceEpoch));
        std::string timeString2 = std::ctime(&(contentCriteria->ifModifiedSince));

        std::cout << "file modified time: " << timeString1 << "\n";
        std::cout << "if modified since time: " << timeString2 << "\n";
        
        if(timeSinceEpoch <= contentCriteria->ifModifiedSince)
        {
            return "Not modified:" + filepath;
        }
    } 

    //check authorization
    std::string ret = authorizationCheck(contentCriteria, filepath);
    if(ret != "ok") {
        std::cout <<  "Unauthorized: " << ret << "\n";
        return "Unauthorized: " + ret;
    }

    return toReturn;
}

int fillInErrorResponse(std::string filepath, HttpResponse* response) {
    if(filepath.find("Not modified") != std::string::npos) {
        std::cout << "not modified\n";
        std::string actual_filepath = filepath.substr(filepath.find(":") + 1);
        (*response).SetStatusCode(myHttpServer::HttpStatusCode::NotModified);
        (*response).SetLastModified(actual_filepath);

        return 1;
    }
    else if (filepath == "NotFound") {
        (*response).SetStatusCode(myHttpServer::HttpStatusCode::NotFound);
        return 1;
    }
    else if (filepath.find("Unauthorized") != std::string::npos) {
        int posOfFirstColon = filepath.find(":");
        int posOfSecondColon = filepath.substr(posOfFirstColon + 1).find(":");
        std::string authType = filepath.substr(posOfFirstColon + 1, posOfSecondColon);
        std::string authName = filepath.substr(posOfSecondColon + posOfFirstColon + 2);
        std::cout << "authType" << authType << "\n";
        std::cout << "authName" << authName << "\n";
        (*response).SetStatusCode(myHttpServer::HttpStatusCode::Unauthorized);
        (*response).SetAuthHeader(authType, authName);
        return 1;
    }
    else if (filepath == "Bad URL") {
        (*response).SetStatusCode(myHttpServer::HttpStatusCode::Forbidden);
        return 1; 
    }
    return 0;
}





////////////////////////////////////////////////////////////
// GET handling                                           
////////////////////////////////////////////////////////////

HttpResponse RequestHandlers::GetHandler(const HttpRequest& request) 
{
    HttpResponse response;
    ContentSelection contentSelectionCriteria;
    std::string root;
    //validate URI
    std::string our_uri = request.uri().path();

    if(our_uri[0] != '/' && our_uri[0] != 'h') {
        std::cerr << "Bad URL1: " << our_uri[0] << "\n";
        fillInErrorResponse("Bad URL", &response);
        return response;
    }
    else if(our_uri.find("..") != std::string::npos) {
        std::cerr << "Bad URL2:" << our_uri << "\n";
        fillInErrorResponse("Bad URL", &response);
        return response;
    }

    //Virtual Host
    //std::cout << "host: " << request.uri().host() << "\n";
    if(virtualHosts.find(request.headers()["Host"]) != virtualHosts.end()) {
        root = virtualHosts[request.headers()["Host"]];
    } 
    else {
        root = virtualHosts["root"]; //unspecified host leads to root virtual host being used
    }

    //Accept Header
    if(!request.headers()["Accept"].empty()) { //request.headers().find("Accept") != request.headers().end())
        //parse accept, we only handle text/html, text/plain, and wildcard for now
        std::string values = request.headers()["Accept"];
        std::stringstream ss(values);

        std::cout << "Accept Header:" << values << "\n";

        std::vector<std::string> tokens;
        std::string token;

        while (std::getline(ss, token, ',')) {
            tokens.push_back(token); //tokens has all of the accepted formats
        }

        for (const std::string& token : tokens) {
           if(token.find(acceptedFormats[0]) != std::string::npos) { //could eventually turn this into a for
                //add HTML to some sort of content generation array
                contentSelectionCriteria.formatArray[0] = 1;
                
           }
           if (token.find(acceptedFormats[1]) != std::string::npos) {
               //add plain to some sort of content generation array
               contentSelectionCriteria.formatArray[1] = 1;
           }
           if (token.find("*/*") != std::string::npos) {
                //std::cout << "wildcard type found\n";
                contentSelectionCriteria.formatArray[2] = 1;
           }
           //if its not one of the type we handle, ignore
        }
    }

    //User-Agent
    if (!request.headers()["User-Agent"].empty()) {//request.headers().find("UserAgent") != request.headers().end())
        // we only handle mobile user or regular user 
        std::string user_agent = request.headers()["User-Agent"];
        //std::cout << "user_agent str: " << user_agent << "\n";
        if(user_agent.find("Mobile") != std::string::npos || 
           user_agent.find("Andriod") != std::string::npos || 
           user_agent.find("iOS") != std::string::npos || 
           user_agent.find("iPhone") != std::string::npos
        ) {
            //add mobile agent to content gen array
            contentSelectionCriteria.userAgent = 1;
        }
        else {
            //add not mobile to some sort of content generation array
            contentSelectionCriteria.userAgent = 0;
        }

    }
    if(!request.headers()["If-Modified-Since"].empty()) {
        //parsing the Http time/date format
        std::istringstream ss(request.headers()["If-Modified-Since"]);
        std::tm timeInfo = {};
        ss.imbue(std::locale("C")); // Set locale to "C" for consistent parsing
        std::cout << "The time:" << request.headers()["If-Modified-Since"];
        ss >> std::get_time(&timeInfo, "%a,%d%b%Y%H:%M:%SGMT");
        if (ss.fail()) {
            std::cerr << "Weird Thing happening w/ time\n";
        }

        std::time_t time = std::mktime(&timeInfo);
        contentSelectionCriteria.ifModifiedSince = time;

    }
    if(!request.headers()["Connection"].empty()) { //request.headers().find("Connection") != request.headers().end())
        std::vector<std::string> connectionAcceptedValues = {"close", "keep-alive"};
        if(request.headers()["Connection"].find(connectionAcceptedValues[0]) != std::string::npos) {
            contentSelectionCriteria.connection = 0;
            response.SetKeepAlive(false);
        }
        else if (request.headers()["Connection"].find(connectionAcceptedValues[1]) != std::string::npos) {
            contentSelectionCriteria.connection = 1;
            response.SetKeepAlive(true);
        }

    }
    if(!request.headers()["Authorization"].empty()) {
        // check that it is the basic form
        std::string authorization_string = request.headers()["Authorization"];
        if (authorization_string.substr(0, 5).compare("Basic") != 0) {
            std::cerr << "Unsupported authentification\n";
        }
        else {
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
    //std::cout << "filepath1: " << filepath << "\n";

    // handle load balance query endpoint 
    if (our_uri == "/load")
    {
        int newRequests = 1000;
        if (newRequests < 0 || newRequests > 100000) {
            response.SetStatusCode(HttpStatusCode::ServiceUnavailable); //503
        }
        else {
            response.SetStatusCode(HttpStatusCode::Ok); //200
        }
        // make wrapper function for headers
        return response; 
    }

    //select what file to send back in http response based on headers
    filepath = contentSelection(request, &response, &contentSelectionCriteria, filepath); // will catch if filepath is invalid
    std::cout << filepath;
    if(fillInErrorResponse(filepath, &response)) {
        std::cout << "Error response\n";
        return response;
    }

    // check if file is CGI and executable 
    if (isExecutable(filepath)) {
        std::cout << "it's executable " << filepath << "\n";
        return runGetExecutable(filepath);
    }

    //get the file contents into a response buffer
    if(filepath.find(".jpg") != std::string::npos)
    {
        printf("JPG IS FOUND \n");
        std::ifstream file(filepath, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Failed to open image file. \n");
        }
        std::vector<char> imageData((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        std::string convertedString(imageData.begin(), imageData.end());
        response.SetContent(imageData, imageData.size(), filepath); 
    }
    else
    {
        std::string data;
        std::string dataChunk;
        std::ifstream fileData(filepath);
        while (std::getline (fileData, dataChunk)) {
            data  = data + dataChunk + "\n";
        }
        response.SetContent(data, filepath);
    }
    
    response.SetStatusCode(HttpStatusCode::Ok);
    
    if(filepath.substr(filepath.length() - 5) == ".html") {
        response.SetHeader("Content-Type", "text/html");
    }
    else if(filepath.substr(filepath.length() - 4) == ".txt") {
        response.SetHeader("Content-Type", "text/plain");
    }
    else if(filepath.substr(filepath.length() - 4) == ".jpg") {
        response.SetHeader("Content-Type", "image/jpeg");
    }

    // return HTTP response object 
    return response;
}

void RequestHandlers::RegisterGetHandlers(HttpServer& server) {
    auto getHandler = [](const HttpRequest& request) -> HttpResponse {
        return RequestHandlers::GetHandler(request);
    };

    server.RegisterHttpRequestHandler(MethodType::GET, getHandler);
}





////////////////////////////////////////////////////////////
// POST handling 
////////////////////////////////////////////////////////////

void RequestHandlers::RegisterPostHandlers(HttpServer& server) {
    auto run_cgi = [] (const HttpRequest& request) -> HttpResponse {
        std::string postData = request.content(); // POST data as stdin for the CGI
        std::string root;
        std::string our_uri = request.uri().path();

        // configure virtual host 
        if(virtualHosts.find(request.uri().host()) != virtualHosts.end()) {
            root = virtualHosts[request.uri().host()];
        }
        else {
            root = virtualHosts["root"]; //unspecified host leads to root virtual host being used
        }
        
        std::string filepath = root + our_uri;
        std::cout << "filepath: " << filepath << "\n";

        // check if the cgiScriptPath is in the list of existing resources
        if(!std::__fs::filesystem::exists(filepath)) {
            std::cerr << "Resource Not Found" << "\n";
            return HttpResponse(HttpStatusCode::NotFound);
        }

        // check if mapped file is executable 
        if (isExecutable(filepath)) {
            std::cout << "it's executable " << filepath << "\n";
            return runPostExecutable(filepath, postData);
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

}