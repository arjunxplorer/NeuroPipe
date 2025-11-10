cmake_minimum_required(VERSION 3.15)
project(NeuroPipe VERSION 1.0.0 LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/lib)
include_directories(${CMAKE_SOURCE_DIR}/third_party/include)

# Find threads library (required for std::thread)
find_package(Threads REQUIRED)

# Broker executable (main server) - New Asio version
add_executable(broker 
    src/broker.cpp 
    src/asio_server.cpp
)
target_link_libraries(broker PRIVATE Threads::Threads)

# Legacy broker (old implementation)
add_executable(broker_legacy 
    src/broker_legacy.cpp 
    src/server.cpp
)
target_link_libraries(broker_legacy PRIVATE Threads::Threads)

# Producer client executable
add_executable(producer_client 
    src/producer.cpp
)
target_link_libraries(producer_client PRIVATE Threads::Threads)

# Consumer client executable
add_executable(consumer_client 
    src/consumer.cpp
)
target_link_libraries(consumer_client PRIVATE Threads::Threads)

# Debug Logger Library
add_library(debug_logger
    lib/debug_logger.cpp
)
target_link_libraries(debug_logger PRIVATE Threads::Threads)

# Example: Simple App with Debug Logging
add_executable(simple_app
    examples/simple_app.cpp
)
target_link_libraries(simple_app PRIVATE debug_logger Threads::Threads)

# Example: Robust App (Graceful Degradation)
add_executable(robust_app
    examples/robust_app.cpp
)
target_link_libraries(robust_app PRIVATE debug_logger Threads::Threads)

# Tests
enable_testing()

add_executable(test_basic 
    tests/test_basic.cpp
)
target_link_libraries(test_basic PRIVATE Threads::Threads)

add_executable(test_asio_broker
    tests/test_asio_broker.cpp
    src/asio_server.cpp
)
target_link_libraries(test_asio_broker PRIVATE Threads::Threads)

add_test(NAME BasicTest COMMAND test_basic)
add_test(NAME AsioTest COMMAND test_asio_broker)

# Install targets
install(TARGETS broker producer_client consumer_client
    RUNTIME DESTINATION bin
)

# Print configuration summary
message(STATUS "")
message(STATUS "NeuroPipe Configuration Summary:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "")

