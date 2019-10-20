cmake_minimum_required(VERSION 3.12)
project(ec_gcm)

set(CMAKE_CXX_STANDARD 17)
set(PTHREAD_LIB pthread)


add_executable(ec_gcm main.cpp
        Manager.cpp                     Manager.h
        container_handler.cpp           container_handler.h
        SubContainer.cpp                SubContainer.h
        ElasticContainer.cpp            ElasticContainer.h
        GlobalCloudManager.cpp          GlobalCloudManager.h
        #Server.cpp                     Server.h

        types/msg.h                     types/msg.cpp
        types/types.h                   types/k_msg.h
        Server.h                        Server.cpp
        Agent.h                         Agent.cpp)

add_executable(tester tests/run_test.cpp
        types/msg.cpp                   types/msg.h
        om.h
        )

add_executable(loop_test tests/run_continuous.cpp
        types/msg.cpp                   types/msg.h
        om.h
        )

target_link_libraries(ec_gcm ${PTHREAD_LIB})