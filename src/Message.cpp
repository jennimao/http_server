#include "Message.h"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <iostream>
#include <ctime>
#include <sstream>
#include <unordered_map>
#include <filesystem>
#include <chrono>
#include <iostream>
#include <cerrno>
#include <sys/stat.h>

namespace fs = std::filesystem;

namespace myHttpServer {

std::string to_string(MethodType method) {
  switch (method) {
    case MethodType::GET:
      return "GET";
    case MethodType::POST:
      return "POST";
    default:
      return std::string();
  }
}

std::string to_string(HttpVersion version) {
  switch (version) {
    case HttpVersion::HTTP_1_1:
      return "HTTP/1.1";
    case HttpVersion::HTTP_1_0:
      return "HTTP/1.0";
    default:
      return std::string();
  }
}

std::string to_string(HttpStatusCode status_code) {
  switch (status_code) {
    case HttpStatusCode::Continue:
      return "Continue";
    case HttpStatusCode::Ok:
      return "OK";
    case HttpStatusCode::Accepted:
      return "Accepted";
    case HttpStatusCode::MovedPermanently:
      return "Moved Permanently";
    case HttpStatusCode::Found:
      return "Found";
    case HttpStatusCode::BadRequest:
      return "Bad Request";
    case HttpStatusCode::Forbidden:
      return "Forbidden";
    case HttpStatusCode::NotFound:
      return "Not Found";
    case HttpStatusCode::MethodNotAllowed:
      return "Method Not Allowed";
    case HttpStatusCode::ImATeapot:
      return "I'm a Teapot";
    case HttpStatusCode::InternalServerError:
      return "Internal Server Error";
    case HttpStatusCode::NotImplemented:
      return "Not Implemented";
    case HttpStatusCode::BadGateway:
      return "Bad Gateway";
    case HttpStatusCode::Unauthorized:
      return "Unauthorized";
    case HttpStatusCode::NotModified:
      return "Not Modified";
    default:
      return std::string();
  }
}

MethodType string_to_method(const std::string& method_string) {
  std::string method_string_uppercase;
  std::transform(method_string.begin(), method_string.end(),
                 std::back_inserter(method_string_uppercase),
                 [](char c) { return toupper(c); });
  if (method_string_uppercase == "GET") {
    return MethodType::GET;
  } else if (method_string_uppercase == "POST") {
    return MethodType::POST;
  } else {
    throw std::invalid_argument("Unexpected HTTP method");
  }
}

HttpVersion string_to_version(const std::string& version_string) {
  std::string version_string_uppercase;
  std::transform(version_string.begin(), version_string.end(),
                 std::back_inserter(version_string_uppercase),
                 [](char c) { return toupper(c); });

  if (version_string_uppercase == "HTTP/1.1") {
    return HttpVersion::HTTP_1_1;
  }
  else if (version_string_uppercase == "HTTP/1.0")
  {
    return HttpVersion::HTTP_1_0;
  } else {
    throw std::invalid_argument("Unexpected HTTP version");
  }
}

std::string to_string(const HttpRequest& request) {
  std::ostringstream oss;

  oss << to_string(request.method()) << ' ';
  oss << request.uri().path() << ' ';
  oss << to_string(request.version()) << "\r\n";
  for (const auto& p : request.headers())
    oss << p.first << ": " << p.second << "\r\n";
  oss << "\r\n";
  oss << request.content();

  return oss.str();
}

std::string to_string(const HttpResponse& response, bool send_content) {
  std::ostringstream oss;

  oss << to_string(response.version()) << ' ';
  oss << static_cast<int>(response.status_code()) << ' ';
  oss << to_string(response.status_code()) << "\r\n";
  for (const auto& p : response.headers())
    oss << p.first << ": " << p.second << "\r\n";
  oss << "\r\n";
  if (send_content) oss << response.content();
  //std::cout << oss.str(); // Print the string to stdout
  return oss.str();
}

HttpRequest string_to_request(const std::string& request_string) {
  std::string start_line, header_lines, message_body;
  std::istringstream iss;
  HttpRequest request;
  std::string line, method, path, version;  // used for first line
  std::string key, value;                   // used for header fields
  Uri uri;
  size_t lpos = 0, rpos = 0;

  rpos = request_string.find("\r\n", lpos);
  if (rpos == std::string::npos) {
    throw std::invalid_argument("Could not find request start line");
  }

  start_line = request_string.substr(lpos, rpos - lpos);
  lpos = rpos + 2;
  rpos = request_string.find("\r\n\r\n", lpos);
  if (rpos != std::string::npos) {  // has header
    header_lines = request_string.substr(lpos, rpos - lpos);
    lpos = rpos + 4;
    rpos = request_string.length();
    if (lpos < rpos) {
      message_body = request_string.substr(lpos, rpos - lpos);
    }
  }

  iss.clear();  // parse the start line
  iss.str(start_line);
  iss >> method >> path >> version;
  if (!iss.good() && !iss.eof()) {
    throw std::invalid_argument("Invalid start line format");
  }
  request.SetMethod(string_to_method(method));
  request.SetUri(Uri(path));
  request.SetVersion(string_to_version(version));
  if (string_to_version(version) != request.version()) {
    //std::cout << "version:" << version << "other:" << to_string(request.version()) << "\n";
    throw std::logic_error("HTTP version not supported");
  }

  iss.clear();  // parse header fields
  iss.str(header_lines);
  while (std::getline(iss, line)) {
    std::istringstream header_stream(line);
    std::getline(header_stream, key, ':');
    std::getline(header_stream, value);

    // remove whitespaces from the two strings
    key.erase(std::remove_if(key.begin(), key.end(),
                             [](char c) { return std::isspace(c); }),
              key.end());
    value.erase(std::remove_if(value.begin(), value.end(),
                               [](char c) { return std::isspace(c); }),
                value.end());
    request.SetHeader(key, value);
  }

  request.SetContent(message_body);

  return request;
}

HttpResponse string_to_response(const std::string& response_string) {
    std::istringstream iss(response_string);
    HttpResponse response;

    // Parse the response status line
    std::string httpVersion;
    int statusCode;
    std::string reasonPhrase;
    iss >> httpVersion >> statusCode;
    std::getline(iss, reasonPhrase);

    // Set the HTTP version and status code in the response
    response.SetStatusCode(static_cast<HttpStatusCode>(statusCode));

    // Parse and set headers
    std::string line;
    while (std::getline(iss, line) && !line.empty()) {
        std::string key, value;
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
        key = line.substr(0, colonPos);
        value = line.substr(colonPos + 1);
        key = key.substr(key.find_first_not_of(" \t"));
        value = value.substr(value.find_first_not_of(" \t"));
        response.SetHeader(key, value);
        }
    }

    // The rest of the input is the message body
    std::string messageBody;
    std::stringstream responseBody;
    while (std::getline(iss, line)) {
        responseBody << line << "\n";
    }
    messageBody = responseBody.str();
    response.SetContent(messageBody);

    return response;
}

std::string HttpResponse::GetCurrentDate() {
  time_t now = time(nullptr);  // Get current timestamp
  struct tm* timeinfo = gmtime(&now);  // Convert to GMT (UTC) time
  char buffer[128];
  // example of HTTP date format: "Tue, 24 Oct 2023 16:17:00 GMT"
  std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", timeinfo);
  return buffer;
}

std::string HttpResponse::GetContentType(const std::string& filename) {
  // map file extensions to content types
  std::unordered_map<std::string, std::string> extensionToContentType = {
      {".html", "text/html"},
      {".txt", "text/plain"},
      {".jpg", "image/jpeg"},
      // Add more mappings as needed
  };

  // extract the file extension from the filename
  size_t dotPosition = filename.find_last_of(".");
  if (dotPosition != std::string::npos) {
      std::string extension = filename.substr(dotPosition);
      auto it = extensionToContentType.find(extension);
      if (it != extensionToContentType.end()) {
          return it->second;
      }
  }
  return "text/plain"; //default, maybe change to unknown
}


std::string HttpResponse::GetLastModified(const std::string& contentPath) {
    // Check if the file exists
    struct stat fileStat;
    if (stat(contentPath.c_str(), &fileStat) != 0) {
        std::cerr << "File not found: " << contentPath << std::endl;
        return "File not found";
    }

    try {
        // Get the last modified time
        std::time_t lastModified = fileStat.st_mtime; // Last modification time in seconds since the epoch

        // Format the last modified time as a string
        std::tm lastModifiedTM = *std::gmtime(&lastModified); // Use gmtime for GMT format
        char buffer[80];
        std::strftime(buffer, 80, "%a, %d %b %Y %H:%M:%S GMT", &lastModifiedTM);

        return buffer;
    } catch (const std::exception& e) {
        std::cerr << "Error getting last modified time: " << e.what() << std::endl;
        return "Error getting last modified time";
    }
}


}  // namespace simple_http_server
