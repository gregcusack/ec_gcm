//
// Created by Greg Cusack on 11/6/19.
//

#include "AgentClient.h"

ec::AgentClient::AgentClient(const ec::Agent *_agent, int _sockfd)
    : agent(_agent), sockfd_new(_sockfd) {}

int64_t ec::AgentClient::send_request(const struct msg_struct::ECMessage &msg) const {
    int64_t ret = -1;
    ec::Facade::ProtoBufFacade::ProtoBuf::sendMessage(sockfd_new, msg);
    msg_struct::ECMessage rx_msg;
    ec::Facade::ProtoBufFacade::ProtoBuf::recvMessage(sockfd_new, rx_msg);
    ret = rx_msg.rsrc_amnt();
    if(msg.request() != rx_msg.request()) {
        std::cout << "sequence number mismatch: (" << msg.request() << ", " << rx_msg.request() << ")" << std::endl;
    }
    return ret;
}