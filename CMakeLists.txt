cmake_minimum_required(VERSION 3.12)
project(TCPClientAndServer)

if (NOT ASIO_DIR)
    message(FATAL_ERROR "You must set ASIO_DIR in order to build the project")
endif()
if (NOT BOOST_SYSTEM_DIR)
    message(FATAL_ERROR "You must set BOOST_SYSTEM_DIR in order to build the project")
endif()
find_package(Threads REQUIRED)
add_executable(TCPClientAndServer main.cpp)
target_include_directories(TCPClientAndServer PRIVATE ${ASIO_DIR}/include ${BOOST_SYSTEM_DIR}/include)
target_compile_definitions(TCPClientAndServer PRIVATE BOOST_ERROR_CODE_HEADER_ONLY)
target_link_libraries(TCPClientAndServer Threads::Threads)
set_target_properties(TCPClientAndServer PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES)
