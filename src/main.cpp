#include <sys/resource.h>
#include <sys/time.h>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <fstream>

#include "Message.h"
#include "Server.h"
#include "Uri.h"
#include "RequestHandlers.h"

using myHttpServer::MethodType;
using myHttpServer::HttpRequest;
using myHttpServer::HttpResponse;
using myHttpServer::HttpServer;
using myHttpServer::HttpStatusCode;
using myHttpServer::RequestHandlers;

void ensure_enough_resource(int resource, std::uint32_t soft_limit, std::uint32_t hard_limit) {
  rlimit new_limit, old_limit;

  new_limit.rlim_cur = soft_limit;
  new_limit.rlim_max = hard_limit;
  getrlimit(resource, &old_limit);

  std::cout << "Old limit: " << old_limit.rlim_cur << " (soft limit), "
            << old_limit.rlim_cur << " (hard limit)." << std::endl;
  std::cout << "New limit: " << new_limit.rlim_cur << " (soft limit), "
            << new_limit.rlim_cur << " (hard limit)." << std::endl;

  if (setrlimit(resource, &new_limit)) {
    std::cerr << "Warning: Could not update resource limit ";
    std::cerr << "(" << strerror(errno) << ")." << std::endl;
    std::cerr << "Consider setting the limit manually with ulimit" << std::endl;
    exit(-1);
  }
}

int main(int argc, char* argv[]) {
  std::string host = "localhost";
  int port = 6789;
  int nselectLoops;
  std::string configFileName;
  
  //parse command line args
  if(argc < 3) {
    std::cerr << "Usage: " << argv[0] << " -config <config_file_name>" << std::endl;
    return 1;
    }

  std::string arg1 = argv[1];
  std::string arg2 = argv[2];

  if (arg1 == "-config") {
    configFileName = arg2;
    std::cout << "Config file name: " << configFileName << std::endl;
  } 
  else {
      std::cerr << "Invalid command line arguments." << std::endl;
      return 1;
    }

  RequestHandlers::ParseConfigFile(configFileName, &port, &nselectLoops);

  //test print everything
  std::cout << "port" << port << "\n";
  std::cout << "nSelectLoops" << nselectLoops << "\n";
  RequestHandlers::PrintVirtualHosts();
  
  HttpServer server(host, port);
  RequestHandlers::RegisterHandlers(server);

  try {
    // std::cout << "Setting new limits for file descriptor count.." <<
    // std::endl; ensure_enough_resource(RLIMIT_NOFILE, 15000, 15000);

    // std::cout << "Setting new limits for number of threads.." << std::endl;
    // ensure_enough_resource(RLIMIT_NPROC, 60000, 60000);

    std::cout << "Starting the web server.." << std::endl;
    server.Start();
    std::cout << " on " << host << ":" << port << std::endl;

    std::cout << "Enter [quit] to stop the server" << std::endl;
    std::string command;
    while (std::cin >> command, command != "quit") {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "'quit' command entered. Stopping the web server.." << std::endl;
    server.Stop();
    std::cout << "Server stopped" << std::endl;
  } catch (std::exception& e) {
    std::cerr << "An error occurred: " << e.what() << std::endl;
    return -1;
  }

  return 0;
}