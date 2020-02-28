#include "../include/ProtoBufFacade.h"

int ec::Facade::ProtoBufFacade::ProtoBuf::sendMessage(const int &sock_fd, const struct msg_struct::ECMessage &msg){
    int tx_size = msg.ByteSizeLong()+4;
    char* tx_buf = new char[tx_size];
    google::protobuf::io::ArrayOutputStream arrayOut(tx_buf, tx_size);
    google::protobuf::io::CodedOutputStream codedOut(&arrayOut);
    codedOut.WriteVarint32(msg.ByteSizeLong());
    msg.SerializeToCodedStream(&codedOut);

    if (write(sock_fd, (void*) tx_buf, tx_size) < 0) {
        std::cout << "[PROTOBUF ERROR]: Error in writing to agent_clients socket: " << sock_fd << std::endl;
        return -1;
    }  
    return tx_size;
}

void ec::Facade::ProtoBufFacade::ProtoBuf::recvMessage(const int &sock_fd, struct msg_struct::ECMessage &rx_msg) {
    char rx_buffer[__BUFFSIZE__];
    bzero(rx_buffer, __BUFFSIZE__);
    int res;

    res = read(sock_fd, rx_buffer, __BUFFSIZE__);
    if (res <= 0) {
        std::cout << "[PROTOBUF ERROR]: Can't read from socket" << std::endl;
    }

    google::protobuf::uint32 size;
    google::protobuf::io::ArrayInputStream ais(rx_buffer,4);
    CodedInputStream coded_input(&ais);
    coded_input.ReadVarint32(&size); 
    
    google::protobuf::io::ArrayInputStream arrayIn(rx_buffer, size);
    google::protobuf::io::CodedInputStream codedIn(&arrayIn);
    codedIn.ReadVarint32(&size);
    google::protobuf::io::CodedInputStream::Limit msgLimit = codedIn.PushLimit(size);
    rx_msg.ParseFromCodedStream(&codedIn);
    codedIn.PopLimit(msgLimit);
}