cmake_minimum_required(VERSION 3.12)
project(ec_gcm)

set(CMAKE_CXX_STANDARD 17)


add_executable(ec_gcm main.cpp
        Manager.cpp                     Manager.h
        container_handler.cpp           container_handler.h
        SubContainer.cpp                SubContainer.h
        ElasticContainer.cpp            ElasticContainer.h

        types/msg.h                     types/msg.cpp
        types/types.h                   types/k_msg.h GlobalCloudManager.cpp GlobalCloudManager.h)