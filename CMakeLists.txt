cmake_minimum_required(VERSION 3.12)
project(ec_gcm)

set(CMAKE_CXX_STANDARD 17)

add_executable(ec_gcm main.cpp
        manager.cpp                     manager.h
        container_handler.cpp           container_handler.h
        container.cpp                   container.h msg.h

)