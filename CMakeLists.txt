cmake_minimum_required(VERSION 3.12)
project(Http)

# Set C++ standard to C++11 (or the desired version).
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Enable testing in CMake.
enable_testing()

# Include Boost.Test.
find_package(Boost REQUIRED COMPONENTS unit_test_framework)
add_library(Boost::Boost INTERFACE IMPORTED)

set(SRC_FILES
    #src/Server.cpp
    src/Main.cpp
    src/Message.cpp 
)

set(INCLUDE_DIRS
    include
    src 
)

# Add test source files.
set(TEST_SRC_FILES
    #test/TestRequestHandler.cpp
    #test/TestServer.cpp
)

add_executable(Http ${SRC_FILES})

# Create the test executable for Boost.Test.
add_executable(HttpServerTests ${TEST_SRC_FILES} ${SRC_FILES})

# Link with Boost.Test and your project's libraries.
target_link_libraries(HttpServerTests 
                      PRIVATE Boost::Boost Boost::unit_test_framework
                      PRIVATE Threads::Threads
                     )
                     
target_link_libraries(Http PRIVATE Threads::Threads)

# Include directories.
target_include_directories(HttpServerTests PRIVATE ${INCLUDE_DIRS})

# Add the test to CMake's testing system.
add_test(NAME HttpServerTests COMMAND HttpServerTests)