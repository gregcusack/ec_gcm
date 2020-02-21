#ifndef PROTO_FACADE_H
#define PROTO_FACADE_H

#include <iostream>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include "../msg.pb.h"
#include "../../om.h"

#define __BUFFSIZE__ 1024

using namespace google::protobuf::io;

class ProtoBufFacade {
    public:
        ProtoBufFacade() {}

        int sendMessage(int sock_fd, msg_struct::ECMessage msg);
        msg_struct::ECMessage recvMessage(int sock_fd);
};

#endif //PROTO_FACADE_H
