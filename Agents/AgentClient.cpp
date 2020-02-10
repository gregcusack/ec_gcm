//
// Created by Greg Cusack on 11/6/19.
//

#include "AgentClient.h"

ec::AgentClient::AgentClient(const ec::Agent *_agent, int _sockfd)
    : agent(_agent), sockfd_new(_sockfd) {

}

std::vector<uint64_t > ec::AgentClient::send_request(struct ec::msg_t* _req) const {
    char buff [__BUFFSIZE__];
    std::vector<uint64_t> ret;
    int32_t bytes_read;

    send(sockfd_new , (char*)_req , sizeof(struct ec::msg_t) , 0 );
    while ( (bytes_read = read( sockfd_new , buff, __BUFFSIZE__)) > 0)
        ret.push_back( ((msg_t*)buff) -> rsrc_amnt);

    return ret;
}