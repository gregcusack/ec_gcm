cmake_minimum_required(VERSION 3.12)
project(ec_gcm)

set(CMAKE_CXX_STANDARD 17)
set(PTHREAD_LIB pthread)
set(ignoreMe "${LOG_CPU_TRACKING}")


include(FindProtobuf)
include(cmake/grpc.cmake)

find_package(Protobuf REQUIRED)
find_package(cpprestsdk REQUIRED)
find_package(Boost REQUIRED system)

add_subdirectory(protoBufSDK)
include_directories(${PROTOBUF_INCLUDE_DIR})

if(NOT TARGET spdlog)
    # Stand-alone build
    find_package(spdlog REQUIRED)
endif()

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

add_custom_command(
        OUTPUT
        ${CMAKE_CURRENT_LIST_DIR}/Agents/containerUpdateGrpc.pb.h
        ${CMAKE_CURRENT_LIST_DIR}/Agents/containerUpdateGrpc.pb.cc
        ${CMAKE_CURRENT_LIST_DIR}/Agents/containerUpdateGrpc.grpc.pb.h
        ${CMAKE_CURRENT_LIST_DIR}/Agents/containerUpdateGrpc.grpc.pb.cc
        MAIN_DEPENDENCY ${CMAKE_CURRENT_LIST_DIR}/Agents/containerUpdateGrpc.proto
        COMMAND ${PROTOBUF_PROTOC}
        ARGS --proto_path=. --cpp_out=. containerUpdateGrpc.proto
        COMMAND ${PROTOBUF_PROTOC}
        ARGS --proto_path=. --grpc_out=. --plugin=protoc-gen-grpc=${GRPC_CPP_GEN} containerUpdateGrpc.proto
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/Agents)

set(GCM_PROTO_SRC
        protoBufSDK/msg.pb.h
        protoBufSDK/msg.pb.cc)

set(GCM_GRPC_SRC
        DeployServerGRPC/deploy.grpc.pb.h
        DeployServerGRPC/deploy.grpc.pb.cc
        DeployServerGRPC/deploy.pb.h
        DeployServerGRPC/deploy.pb.cc
        DeployServerGRPC/DeployerExportServiceImpl.cpp DeployServerGRPC/DeployerExportServiceImpl.h

        Agents/containerUpdateGrpc.grpc.pb.h
        Agents/containerUpdateGrpc.grpc.pb.cc
        Agents/containerUpdateGrpc.pb.h
        Agents/containerUpdateGrpc.pb.cc

        )


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
        ${GCM_CORE} types/ports.h)

add_executable(grpc_async_test tests/grpc_test_async.cpp
        ${GCM_CORE} )

add_executable(deploy_test tests/grpc_test.cpp
        ${GCM_CORE})


add_executable(tester tests/run_test.cpp
        types/msg.cpp                   types/msg.h
        om.h
        )

add_executable(loop_test tests/run_continuous.cpp
        types/msg.cpp                   types/msg.h
        om.h)


target_link_libraries(ec_gcm PRIVATE ${PTHREAD_LIB} -lssl -lcrypto cpprestsdk::cpprest ${Boost_SYSTEM_LIBRARY} proto ${PROTOBUF_LIBRARY} ${GRPC_LIB} spdlog::spdlog)
target_link_libraries(deploy_test PRIVATE ${PTHREAD_LIB} -lssl -lcrypto  cpprestsdk::cpprest ${Boost_SYSTEM_LIBRARY} proto ${PROTOBUF_LIBRARY} ${GRPC_LIB} spdlog::spdlog)
target_link_libraries(grpc_async_test PRIVATE ${PTHREAD_LIB} -lssl -lcrypto cpprestsdk::cpprest ${Boost_SYSTEM_LIBRARY} proto ${PROTOBUF_LIBRARY} ${GRPC_LIB} spdlog::spdlog)

if(DEFINED WITH_DEBUG)
    if(WITH_DEBUG MATCHES DEBUG OR WITH_DEBUG MATCHES debug)
        message(STATUS "LOG LEVEL --> DEBUG")
        target_compile_definitions(ec_gcm PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
    elseif(WITH_DEBUG MATCHES TRACE OR WITH_DEBUG MATCHES trace)
        message(STATUS "LOG LEVEL --> TRACE")
        target_compile_definitions(ec_gcm PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)
    elseif(WITH_DEBUG MATCHES INFO OR WITH_DEBUG MATCHES error)
        message(STATUS "LOG LEVEL --> ERROR")
        target_compile_definitions(ec_gcm PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_ERROR)
    elseif(WITH_DEBUG MATCHES INFO OR WITH_DEBUG MATCHES info)
        message(STATUS "LOG LEVEL --> INFO")
        target_compile_definitions(ec_gcm PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
    elseif(WITH_DEBUG MATCHES CRITICAL OR WITH_DEBUG MATCHES critical)
        message(STATUS "LOG LEVEL --> CRITICAL")
        target_compile_definitions(ec_gcm PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_CRITICAL)
    else()
        message(STATUS "LOG LEVEL Default --> DEBUG")
        target_compile_definitions(ec_gcm PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
    endif()
elseif(CMAKE_BUILD_TYPE MATCHES Debug OR CMAKE_BUILD_TYPE MATCHES debug OR CMAKE_BUILD_TYPE MATCHES DEBUG)
    message(STATUS "LOG LEVEL Default --> DEBUG")
    target_compile_definitions(ec_gcm PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
else()
    message(STATUS "LOG LEVEL --> INFO")
    target_compile_definitions(ec_gcm PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
endif()