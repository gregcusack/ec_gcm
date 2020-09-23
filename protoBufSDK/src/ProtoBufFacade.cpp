#include "../include/ProtoBufFacade.h"

int ec::Facade::ProtoBufFacade::ProtoBuf::sendMessage(const int &sock_fd, const struct msg_struct::ECMessage &msg){
    int tx_size = msg.ByteSizeLong()+4;
    char* tx_buf = new char[tx_size];
    google::protobuf::io::ArrayOutputStream arrayOut(tx_buf, tx_size);
    google::protobuf::io::CodedOutputStream codedOut(&arrayOut);
    codedOut.WriteVarint32(msg.ByteSizeLong());
    msg.SerializeToCodedStream(&codedOut);
//    std::cout << "[PROTOBUF log]: TX Message Body length size: " << tx_size-4 << std::endl;
    if (write(sock_fd, (void*) tx_buf, tx_size) < 0) {
        SPDLOG_ERROR("Error in writing to agent_clients socket: {}", sock_fd);
        return -1;
    }
    delete[] tx_buf;
    return tx_size;
}

void ec::Facade::ProtoBufFacade::ProtoBuf::recvMessage(const int &sock_fd, struct msg_struct::ECMessage &rx_msg) {
    char rx_buffer[__BUFFSIZE__];
    bzero(rx_buffer, __BUFFSIZE__);
    int res;

    res = read(sock_fd, rx_buffer, __BUFFSIZE__);
    if (res <= 0) {
        SPDLOG_ERROR("Can't read from socket");
    }

    google::protobuf::io::ArrayInputStream arrayIn(rx_buffer, res);
    google::protobuf::io::CodedInputStream codedIn(&arrayIn);
    google::protobuf::io::CodedInputStream::Limit msgLimit = codedIn.PushLimit(res);
    rx_msg.ParseFromCodedStream(&codedIn);
    codedIn.PopLimit(msgLimit);
}