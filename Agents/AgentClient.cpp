//
// Created by Greg Cusack on 11/6/19.
//

#include "AgentClient.h"

ec::rpc::AgentClient::AgentClient(const ec::Agent *_agent, const std::shared_ptr<grpc::Channel>& channel)
    : stub_(ec::rpc::containerUpdate::ContainerUpdateHandler::NewStub(channel)), agent(_agent), test_cc(0) {

    SPDLOG_DEBUG("agentclient constructor");
    incr_test_cc();
    auto grpc_addr = agent->get_ip().to_string() + ":" + std::to_string(agent->get_port());
    SPDLOG_DEBUG("agentclient connection ip:port: {}", grpc_addr);
    thr_quota_ = std::thread(&ec::rpc::AgentClient::AsyncCompleteRpcQuota, this);
    thr_resize_mem_ = std::thread(&ec::rpc::AgentClient::AsyncCompleteRpcResizeMemLimitPages, this);
    thr_get_mem_usage = std::thread(&ec::rpc::AgentClient::AsyncCompleteRpcGetMemUsageBytes, this);
    thr_get_mem_limit = std::thread(&ec::rpc::AgentClient::AsyncCompleteRpcGetMemLimitBytes, this);

}


int ec::rpc::AgentClient::updateContainerQuota(uint32_t cgroup_id, uint64_t new_quota, const std::string& change, uint32_t seq_num) {
    ec::rpc::containerUpdate::ContainerQuotaRequest request;
    request.set_cgroupid(int32_t(cgroup_id));
    request.set_newquota(new_quota);
    request.set_resizeflag(change);
    request.set_sequencenum(int32_t(seq_num));

    auto *call = new AsyncClientCallQuota;
    call->request.set_sequencenum(int32_t(seq_num));
    call->response_reader =
            stub_->PrepareAsyncReqQuotaUpdate(&call->context, request, &cq_quota_);
    call->response_reader->StartCall();
    call->response_reader->Finish(&call->reply, &call->status, (void*)call);

    return call->status.error_code(); //always zero
//    return 0;

}

/* TODO: Error handling here not great. updateContainerQuota() will always return 0 even if thread below
 * fails since it's async. not exactly sure how to handle this
 */
void ec::rpc::AgentClient::AsyncCompleteRpcQuota() {
    void *got_tag;
    bool ok = false;
    incr_test_cc();
    while(cq_quota_.Next(&got_tag, &ok)) {
        incr_test_cc();
        auto *call = static_cast<AsyncClientCallQuota*>(got_tag);

//        GPR_ASSERT(ok);

        if(call->status.ok()) {
            if(call->reply.sequencenum() != call->request.sequencenum()) {
                SPDLOG_ERROR("seq nums don't match in updateConatiner quota! tx, rx: {}, {}",
                             call->request.sequencenum(), call->reply.sequencenum());
            }
        }
        else {
            SPDLOG_ERROR("RPC failed");
            SPDLOG_ERROR("status: {}", call->status.error_message());
            SPDLOG_ERROR("error code: {}", call->status.error_code());
            SPDLOG_ERROR("details: {}", call->status.error_details());
        }
        delete call;
    }
}

int64_t ec::rpc::AgentClient::resizeMemoryLimitPages(uint32_t cgroup_id, uint64_t new_mem_limit) {
    ec::rpc::containerUpdate::ResizeMaxMemRequest txMsg;
    txMsg.set_cgroupid(int32_t(cgroup_id));
    txMsg.set_newmemlimit(new_mem_limit);

    auto *call = new AsyncClientCallResizeMemLimitPages;
    call->response_reader = stub_->PrepareAsyncReqResizeMaxMem(&call->context, txMsg, &cq_resize_mem_);
    call->response_reader->Finish(&call->reply, &call->status, (void*)call);

//    return 0;
    return call->reply.errorcode();
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

//    return 0;
    return call->reply.memusage();
}

int64_t ec::rpc::AgentClient::getMemoryLimitBytes(uint32_t cgroup_id) {
    grpc::ClientContext context;
    ec::rpc::containerUpdate::CgroupId txMsg;
    txMsg.set_cgroupid(int32_t(cgroup_id));

    auto *call = new AsyncClientCallGetMemLimitBytes;
    call->response_reader = stub_->PrepareAsyncReadMemLimit(&call->context, txMsg, &cq_get_mem_lim_);
    call->response_reader->Finish(&call->reply, &call->status, (void*)call);

//    return 0;
    return call->reply.memlimit();
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

