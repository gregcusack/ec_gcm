cmake_minimum_required(VERSION 3.12)
project(ec_gcm)

set(CMAKE_CXX_STANDARD 17)
set(PTHREAD_LIB pthread)

include(FindProtobuf)
include(cmake/grpc.cmake)

find_package(Protobuf REQUIRED)
find_package(cpprestsdk REQUIRED)
find_package(Boost REQUIRED system)

add_subdirectory(protoBufSDK)
include_directories(${PROTOBUF_INCLUDE_DIR})

add_custom_command(
        OUTPUT
        ${CMAKE_CURRENT_LIST_DIR}/protoBufSDK/msg.pb.h
        ${CMAKE_CURRENT_LIST_DIR}/protoBufSDK/msg.pb.cc
        MAIN_DEPENDENCY ${CMAKE_CURRENT_LIST_DIR}/protoBufSDK/msg.proto
        COMMAND ${PROTOBUF_PROTOC}
        ARGS --proto_path=. --cpp_out=. msg.proto
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/protoBufSDK)

add_custom_command(
        OUTPUT
        ${CMAKE_CURRENT_LIST_DIR}/DeployServerGRPC/deploy.pb.h
        ${CMAKE_CURRENT_LIST_DIR}/DeployServerGRPC/deploy.pb.cc
        ${CMAKE_CURRENT_LIST_DIR}/DeployServerGRPC/deploy.grpc.pb.h
        ${CMAKE_CURRENT_LIST_DIR}/DeployServerGRPC/deploy.grpc.pb.cc
        MAIN_DEPENDENCY ${CMAKE_CURRENT_LIST_DIR}/DeployServerGRPC/deploy.proto
        COMMAND ${PROTOBUF_PROTOC}
        ARGS --proto_path=. --cpp_out=. deploy.proto
        COMMAND ${PROTOBUF_PROTOC}
        ARGS --proto_path=. --grpc_out=. --plugin=protoc-gen-grpc=${GRPC_CPP_GEN} deploy.proto
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/DeployServerGRPC)

set(GCM_PROTO_SRC
        protoBufSDK/msg.pb.h
        protoBufSDK/msg.pb.cc)

set(GCM_GRPC_SRC
        DeployServerGRPC/deploy.grpc.pb.h
        DeployServerGRPC/deploy.grpc.pb.cc
        DeployServerGRPC/deploy.pb.h
        DeployServerGRPC/deploy.pb.cc
        DeployServerGRPC/DeployerExportServiceImpl.cpp DeployServerGRPC/DeployerExportServiceImpl.h)

set(AGENTS
        Agents/Agent.h                          Agents/Agent.cpp
        Agents/AgentClientDB.h                  Agents/AgentClientDB.cpp
        Agents/AgentClient.h                    Agents/AgentClient.cpp
        )

set(STATS
        stats/local/cpu_l.h                     stats/local/cpu_l.cpp
        stats/local/mem_l.h                     stats/local/mem_l.cpp
        stats/global/cpu_g.h                    stats/global/cpu_g.cpp
        stats/global/mem_g.h                    stats/global/mem_g.cpp
        )

set(GCM_CORE
        ElasticContainer.h                      ElasticContainer.cpp
        SubContainer.h                          SubContainer.cpp
        ECAPI.h                                 ECAPI.cpp
        GlobalControlManager.h                  GlobalControlManager.cpp
        types/msg.h                             types/msg.cpp
        types/types.h                           types/k_msg.h
        Server.h                                Server.cpp
        Manager.h                               Manager.cpp
        jsonSDK/include/JSONFacade.h            jsonSDK/src/JSONFacade.cpp
        protoBufSDK/include/ProtoBufFacade.h    protoBufSDK/src/ProtoBufFacade.cpp
        cAdvisorSDK/include/cAdvisorFacade.h    cAdvisorSDK/src/cAdvisorFacade.cpp
        stats/window/Window.h                   stats/window/Window_impl.h
        ${GCM_PROTO_SRC}    ${GCM_GRPC_SRC} ${AGENTS} ${STATS}
        )

add_executable(ec_gcm main.cpp
        ${GCM_CORE})

add_executable(deploy_test tests/grpc_test.cpp
        ${GCM_CORE})

add_executable(tester tests/run_test.cpp
        types/msg.cpp                   types/msg.h
        om.h
        )

add_executable(loop_test tests/run_continuous.cpp
        types/msg.cpp                   types/msg.h
        om.h)

target_link_libraries(ec_gcm PRIVATE ${PTHREAD_LIB} -lssl -lcrypto cpprestsdk::cpprest ${Boost_SYSTEM_LIBRARY} proto ${PROTOBUF_LIBRARY} ${GRPC_LIB})
target_link_libraries(deploy_test PRIVATE ${PTHREAD_LIB} -lssl -lcrypto  cpprestsdk::cpprest ${Boost_SYSTEM_LIBRARY} proto ${PROTOBUF_LIBRARY} ${GRPC_LIB})

if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(ec_gcm PUBLIC DEBUG=1)
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_definitions(ec_gcm PUBLIC DEBUG=0)
    target_compile_definitions(ec_gcm PUBLIC DEBUG_MAX=0)
endif()