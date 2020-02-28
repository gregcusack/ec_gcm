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

namespace ec {
    namespace Facade {
        namespace ProtoBufFacade {
            class ProtoBuf {
            public:
                int sendMessage(const int &sock_fd, const msg_struct::ECMessage &msg);
                void recvMessage(const int &sock_fd,  msg_struct::ECMessage &rx_msg);
            };
        }
    }
}
#endif //PROTO_FACADE_H
