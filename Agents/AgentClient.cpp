//
// Created by Greg Cusack on 11/6/19.
//

#include "AgentClient.h"

ec::AgentClient::AgentClient(const ec::Agent *_agent, int _sockfd)
    : agent(_agent), sockfd_new(_sockfd) {

}

int64_t ec::AgentClient::send_request(const struct msg_struct::ECMessage &msg) const {
    
    int tx_size = msg.ByteSizeLong()+4;
    char* tx_buf = new char[tx_size];
    google::protobuf::io::ArrayOutputStream arrayOut(tx_buf, tx_size);
    google::protobuf::io::CodedOutputStream codedOut(&arrayOut);
    codedOut.WriteVarint32(msg.ByteSizeLong());
    msg.SerializeToCodedStream(&codedOut);

    char rx_buffer[__BUFFSIZE__];
    int64_t ret = -1;
    int32_t bytes_read;
//    std::cerr << "[dbg] send_request: asking question from the agent and socket file descriptor is: " <<  sockfd_new << " with request type: " << msg.req_type() << std::endl;
    if(send(sockfd_new , (void *)tx_buf , tx_size , 0 ) <= 0) {
        std::cerr << "[ERROR]: Agent client failed sending protobuf to agent!" << std::endl;
    }

    if ( (bytes_read = read( sockfd_new , rx_buffer, __BUFFSIZE__)) > 0){

        msg_struct::ECMessage rx_msg;
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

//        std::cerr << "[dbg] send_request: Agent returned rsrc amnt: " << rx_msg.rsrc_amnt() <<" \n";
        ret = rx_msg.rsrc_amnt();
    }

    return ret;
}