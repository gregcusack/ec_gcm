//
// Created by Greg Cusack on 11/6/19.
//

#include "AgentClient.h"

ec::rpc::AgentClient::AgentClient(const ec::Agent *_agent) : agent(_agent) {}

int ec::rpc::AgentClient::connectAgentGrpc() {

    auto grpc_addr = agent->get_ip().to_string() + ":" + std::to_string(agent->get_port());
    SPDLOG_DEBUG("agentclient connection ip:port: {}", grpc_addr);

    try {
        channel_ = grpc::CreateChannel(grpc_addr, grpc::InsecureChannelCredentials());
    }
    catch (std::exception &e) {
        SPDLOG_ERROR("[ERROR] AgentClient: Connection to agent_client {} failed. error code: {}", agent->get_ip().to_string(), e.what());
//        std::cerr << "connectAgentGrpc() Error code: " << e.what() << std::endl;
        return -1;
    }

    stub_ = ec::rpc::containerUpdate::ContainerUpdateHandler::NewStub(channel_);
    return 0;
}

int ec::rpc::AgentClient::updateContainerQuota(uint32_t cgroup_id, uint64_t new_quota, const std::string& change, uint32_t seq_num) {

    grpc::ClientContext context;
    ec::rpc::containerUpdate::ContainerQuotaRequest txMsg;
    txMsg.set_cgroupid(int32_t(cgroup_id));
    txMsg.set_newquota(new_quota);
    txMsg.set_resizeflag(change);
    txMsg.set_sequencenum(int32_t(seq_num));

    ec::rpc::containerUpdate::ContainerQuotaReply rxMsg;
    grpc::Status status = stub_->ReqQuotaUpdate(&context, txMsg, &rxMsg);

    if(!status.ok()) {
        std::cout << "status: " << status.error_message() << std::endl;
        std::cout << "error code: " << status.error_code() << std::endl;
        std::cout << "details: " << status.error_details() << std::endl;
    }
//    SPDLOG_TRACE("updateContainerQuota rx: {}, {}, {}", rxMsg.cgroupid(), rxMsg.updatequota(), rxMsg.errorcode());

    if(txMsg.sequencenum() != rxMsg.sequencenum()) {
        SPDLOG_ERROR("seq nums don't match in updateConatiner quota! tx, rx: {}, {}", txMsg.sequencenum(), rxMsg.sequencenum());
    }

    return rxMsg.errorcode();
}

int64_t ec::rpc::AgentClient::resizeMemoryLimitPages(uint32_t cgroup_id, uint64_t new_mem_limit) {
    grpc::ClientContext context;
    ec::rpc::containerUpdate::ResizeMaxMemRequest txMsg;
    txMsg.set_cgroupid(int32_t(cgroup_id));
    txMsg.set_newmemlimit(new_mem_limit);

    ec::rpc::containerUpdate::ResizeMaxMemReply rxMsg;
    grpc::Status status = stub_->ReqResizeMaxMem(&context, txMsg, &rxMsg);

    if(status.ok()) {
        SPDLOG_DEBUG("resizeMemoryLimitPages rx: {}, {}", rxMsg.cgroupid(), rxMsg.errorcode());
    }
    else {
        std::cout << "status: " << status.error_message() << std::endl;
        std::cout << "error code: " << status.error_code() << std::endl;
        std::cout << "details: " << status.error_details() << std::endl;
    }
    return rxMsg.errorcode();
}

int64_t ec::rpc::AgentClient::getMemoryUsageBytes(uint32_t cgroup_id) {
    grpc::ClientContext context;
    ec::rpc::containerUpdate::CgroupId txMsg;
    txMsg.set_cgroupid(int32_t(cgroup_id));

    ec::rpc::containerUpdate::ReadMemUsageReply rxMsg;
    grpc::Status status = stub_->ReadMemUsage(&context, txMsg, &rxMsg);

    if (status.ok()) {
        SPDLOG_DEBUG("getMemoryUsageBytes rx: {}, {}", rxMsg.cgroupid(), rxMsg.memusage());
    } else {
        std::cout << "status: " << status.error_message() << std::endl;
        std::cout << "error code: " << status.error_code() << std::endl;
        std::cout << "details: " << status.error_details() << std::endl;
    }
    return rxMsg.memusage();
}

int64_t ec::rpc::AgentClient::getMemoryLimitBytes(uint32_t cgroup_id) {
    grpc::ClientContext context;
    ec::rpc::containerUpdate::CgroupId txMsg;
    txMsg.set_cgroupid(int32_t(cgroup_id));

    ec::rpc::containerUpdate::ReadMemLimitReply rxMsg;
    grpc::Status status = stub_->ReadMemLimit(&context, txMsg, &rxMsg);

    if (status.ok()) {
        SPDLOG_DEBUG("getMemoryUsageLimit rx: {}, {}", rxMsg.cgroupid(), rxMsg.memlimit());
    } else {
        std::cout << "status: " << status.error_message() << std::endl;
        std::cout << "error code: " << status.error_code() << std::endl;
        std::cout << "details: " << status.error_details() << std::endl;
    }
    return rxMsg.memlimit();
}




//int64_t ec::rpc::AgentClient::send_request(const struct msg_struct::ECMessage &msg) { //const {
//    int64_t ret;
//    std::unique_lock<std::mutex> lk(sendlock);
//    ec::Facade::ProtoBufFacade::ProtoBuf::sendMessage(sockfd_new, msg);
//    msg_struct::ECMessage rx_msg;
//    ec::Facade::ProtoBufFacade::ProtoBuf::recvMessage(sockfd_new, rx_msg);
//    ret = rx_msg.rsrc_amnt();
//    if(msg.request() != rx_msg.request()) {
//        SPDLOG_ERROR("sequence number mismatch: ({}, {})", msg.request(), rx_msg.request());
//    }
//    return ret;
//}
//
