//
// Created by Greg Cusack on 11/6/19.
//

#include "AgentClient.h"

ec::AgentClient::AgentClient(const ec::Agent *_agent, int _sockfd)
    : agent(_agent), sockfd_new(_sockfd) {

}

uint64_t ec::AgentClient::send_request(const struct msg_struct::ECMessage &msg) const {
    int status;    
    ec::Facade::ProtoBufFacade::ProtoBuf message;
    status = message.sendMessage(sockfd_new, msg);
    msg_struct::ECMessage rx_msg;
    message.recvMessage(sockfd_new, rx_msg);
    return rx_msg.rsrc_amnt();
}