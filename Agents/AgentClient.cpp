//
// Created by Greg Cusack on 11/6/19.
//

#include "AgentClient.h"

ec::AgentClient::AgentClient(const ec::Agent *_agent, int _sockfd)
    : agent(_agent), sockfd_new(_sockfd) {

}

int64_t ec::AgentClient::send_request(const struct msg_struct::ECMessage &msg) {
    sendlock.lock();
    int status;
//    std::cout << " ################ " << std::endl;
//    std::cout << "in (quota, seq): (" << msg.quota() << ", " << msg.request() << ")" << std::endl;
//    auto t1 = std::chrono::high_resolution_clock::now();
    int64_t ret = -1;
    ec::Facade::ProtoBufFacade::ProtoBuf message;
    status = message.sendMessage(sockfd_new, msg);
    msg_struct::ECMessage rx_msg;
    message.recvMessage(sockfd_new, rx_msg);
//    auto t2 = std::chrono::high_resolution_clock::now();
//    std::cout << "duration: " << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count() << std::endl;
    ret = rx_msg.rsrc_amnt();
//    std::cout << "out (quota, seq): (" << rx_msg.quota() << ", " << rx_msg.request() << ")" << std::endl;
    if(msg.request() != rx_msg.request()) {
        std::cout << "sequence number mismatch: (" << msg.request() << ", " << rx_msg.request() << ")" << std::endl;
    }
//    else {
//        std::cout << "sequence numbers match: (" << msg.request() << ", " << rx_msg.request() << ")" << std::endl;
//    }
//    std::cout << "send_request ret val: " << ret << std::endl;
    sendlock.unlock();
    return ret;
}