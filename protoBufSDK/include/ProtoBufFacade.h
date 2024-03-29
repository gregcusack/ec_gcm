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
#include "spdlog/spdlog.h"

#define __BUFFSIZE__ 1024

using namespace google::protobuf::io;

namespace ec {
    namespace Facade {
        namespace ProtoBufFacade {
            class ProtoBuf {
            public:
                static int sendMessage(const int &sock_fd, const struct msg_struct::ECMessage &msg);
                static void recvMessage(const int &sock_fd,  struct msg_struct::ECMessage &rx_msg);
            };
        }
    }
}
#endif //PROTO_FACADE_H
