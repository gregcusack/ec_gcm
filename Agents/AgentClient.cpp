//
// Created by Greg Cusack on 11/6/19.
//

#include "AgentClient.h"

ec::rpc::AgentClient::AgentClient(const ec::Agent *_agent) : agent(_agent) {}

int ec::rpc::AgentClient::connectAgentGrpc() {

    auto grpc_addr = agent->get_ip().to_string() + ":" + std::to_string(agent->get_port());
    SPDLOG_DEBUG("agentclient connection ip:port: {}", grpc_addr);

    channel_ = grpc::CreateChannel(grpc_addr, grpc::InsecureChannelCredentials());
    stub_ = ec::rpc::containerUpdate::ContainerUpdateHandler::NewStub(channel_);

    thread_test = std::thread(&ec::rpc::AgentClient::AsyncCompleteRpcQuota, this);
    return 0;
}



int ec::rpc::AgentClient::updateContainerQuota(uint32_t cgroup_id, uint64_t new_quota, const std::string& change, uint32_t seq_num) {
    SPDLOG_DEBUG("suh");
    ec::rpc::containerUpdate::ContainerQuotaRequest txMsg;
    txMsg.set_cgroupid(int32_t(cgroup_id));
    txMsg.set_newquota(new_quota);
    txMsg.set_resizeflag(change);
    txMsg.set_sequencenum(int32_t(seq_num));
    SPDLOG_DEBUG("deets: cgid, new_quota, change, seq_num: {}, {}, {}, {}", cgroup_id, new_quota, change, seq_num);


    auto *call = new AsyncClientCallQuota;
    SPDLOG_DEBUG("suh");
    call->response_reader = stub_->PrepareAsyncReqQuotaUpdate(&call->context, txMsg, &cq_quota_);
    SPDLOG_DEBUG("suh");
    if(!call->response_reader) {
        SPDLOG_ERROR("error!");
    }

    call->response_reader->StartCall();
    SPDLOG_DEBUG("suh");
    call->response_reader->Finish(&call->reply, &call->status, (void*)call);
    SPDLOG_DEBUG("suh");
    return 0;
//    return call->reply.errorcode();

}

int64_t ec::rpc::AgentClient::resizeMemoryLimitPages(uint32_t cgroup_id, uint64_t new_mem_limit) {
    ec::rpc::containerUpdate::ResizeMaxMemRequest txMsg;
    txMsg.set_cgroupid(int32_t(cgroup_id));
    txMsg.set_newmemlimit(new_mem_limit);

    auto *call = new AsyncClientCallResizeMemLimitPages;
    call->response_reader = stub_->PrepareAsyncReqResizeMaxMem(&call->context, txMsg, &cq_resize_mem_);
    call->response_reader->Finish(&call->reply, &call->status, (void*)call);

    return 0;
//    return call->reply.errorcode();
}

int64_t ec::rpc::AgentClient::getMemoryUsageBytes(uint32_t cgroup_id) {
    auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(1000); //wait max 1 second

//    context.set_deadline(deadline);
    ec::rpc::containerUpdate::CgroupId txMsg;
    txMsg.set_cgroupid(int32_t(cgroup_id));
    ec::rpc::containerUpdate::ReadMemUsageReply rxMsg;

    auto *call = new AsyncClientCallGetMemUsageBytes;
    call->context.set_deadline(deadline);
    call->response_reader = stub_->PrepareAsyncReadMemUsage(&call->context, txMsg, &cq_get_mem_usage_);
    call->response_reader->Finish(&call->reply, &call->status, (void*)call);

    return 0;
//    return call->reply.memusage();
}

int64_t ec::rpc::AgentClient::getMemoryLimitBytes(uint32_t cgroup_id) {
    grpc::ClientContext context;
    ec::rpc::containerUpdate::CgroupId txMsg;
    txMsg.set_cgroupid(int32_t(cgroup_id));

    auto *call = new AsyncClientCallGetMemLimitBytes;
    call->response_reader = stub_->PrepareAsyncReadMemLimit(&call->context, txMsg, &cq_get_mem_lim_);
    call->response_reader->Finish(&call->reply, &call->status, (void*)call);

    return 0;
//    return call->reply.memlimit();
}

void ec::rpc::AgentClient::AsyncCompleteRpcQuota() {
    void *got_tag;
    bool ok = false;
    std::cout << "suh1" << std::endl;

    while(cq_quota_.Next(&got_tag, &ok)) {
        std::cout << "suh2" << std::endl;
        auto *call = static_cast<AsyncClientCallQuota*>(got_tag);
        std::cout << "suh3" << std::endl;

//        GPR_ASSERT(ok);

        if(call->status.ok()) {
            SPDLOG_DEBUG("suh");
            SPDLOG_DEBUG("rx: {}", call->reply.cgroupid());
            if(call->reply.sequencenum() != call->request.sequencenum()) {
                SPDLOG_ERROR("seq nums don't match in updateConatiner quota! tx, rx: {}, {}",
                             call->request.sequencenum(), call->reply.sequencenum());
            }

        } else {
            SPDLOG_ERROR("RPC failed");
            SPDLOG_ERROR("status: {}", call->status.error_message());
            SPDLOG_ERROR("error code: {}", call->status.error_code());
            SPDLOG_ERROR("details: {}", call->status.error_details());
        }
        std::cout << "suh4" << std::endl;

        delete call;
    }
}

void ec::rpc::AgentClient::AsyncCompleteRpcResizeMemLimitPages() {
    void *got_tag;
    bool ok = false;

    while(cq_resize_mem_.Next(&got_tag, &ok)) {
        auto *call = static_cast<AsyncClientCallResizeMemLimitPages*>(got_tag);

//        GPR_ASSERT(ok);

        if(call->status.ok()) {
//            SPDLOG_DEBUG("rx: {}", call->reply.message());
            SPDLOG_DEBUG("resizeMemoryLimitPages rx: {}, {}", call->reply.cgroupid(), call->reply.errorcode());
        } else {
            SPDLOG_ERROR("RPC failed");
            SPDLOG_ERROR("status: {}", call->status.error_message());
            SPDLOG_ERROR("error code: {}", call->status.error_code());
            SPDLOG_ERROR("details: {}", call->status.error_details());
        }
        delete call;
    }
}

void ec::rpc::AgentClient::AsyncCompleteRpcGetMemUsageBytes() {
    void *got_tag;
    bool ok = false;

    while(cq_get_mem_usage_.Next(&got_tag, &ok)) {
        auto *call = static_cast<AsyncClientCallGetMemUsageBytes*>(got_tag);

//        GPR_ASSERT(ok);

        if(call->status.ok()) {
            SPDLOG_DEBUG("getMemoryUsageBytes rx: {}, {}", call->reply.cgroupid(), call->reply.memusage());
        } else {
            SPDLOG_ERROR("RPC failed");
            SPDLOG_ERROR("status: {}", call->status.error_message());
            SPDLOG_ERROR("error code: {}", call->status.error_code());
            SPDLOG_ERROR("details: {}", call->status.error_details());
        }
        delete call;
    }
}

void ec::rpc::AgentClient::AsyncCompleteRpcGetMemLimitBytes() {
    void *got_tag;
    bool ok = false;

    while(cq_get_mem_lim_.Next(&got_tag, &ok)) {
        auto *call = static_cast<AsyncClientCallGetMemLimitBytes*>(got_tag);

//        GPR_ASSERT(ok);

        if(call->status.ok()) {
            SPDLOG_DEBUG("getMemoryUsageLimit rx: {}, {}", call->reply.cgroupid(), call->reply.memlimit());
        } else {
            SPDLOG_ERROR("RPC failed");
            SPDLOG_ERROR("status: {}", call->status.error_message());
            SPDLOG_ERROR("error code: {}", call->status.error_code());
            SPDLOG_ERROR("details: {}", call->status.error_details());
        }
        delete call;
    }
}

