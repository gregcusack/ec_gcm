find_library(GRPC_LIB grpc++)
find_path(GRPC_INC grpc++/grpc++.h)
find_program(GRPC_CPP_GEN grpc_cpp_plugin)

if(NOT GRPC_LIB OR NOT GRPC_INC OR NOT GRPC_CPP_GEN)
    set(WITH_GRPC FALSE)
    message(ERROR "Detecting libgrpc++: not found - disable support")
else()
    add_definitions(-DWITH_GRPC=1)
    message(STATUS "Detecting libgrpc++:
    GRPC_LIB: ${GRPC_LIB},
    GRPC_INC: ${GRPC_INC},
    GRPC_CPP_GEN: ${GRPC_CPP_GEN} - done")
endif (NOT GRPC_LIB OR NOT GRPC_INC OR NOT GRPC_CPP_GEN)