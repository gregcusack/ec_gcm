//
// Created by greg on 4/30/20.
//

#include <grpc++/grpc++.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include "../DeployServerGRPC/deploy.grpc.pb.h"
#include "../DeployServerGRPC/DeployerExportServiceImpl.h"
#include "../Agents/containerUpdateGrpc.grpc.pb.h"
#include "../Agents/AgentClientDB.h"

ec::AgentClientDB* ec::AgentClientDB::agent_clients_db_instance = nullptr;

class AyncGreeterClient {
public:
    explicit AyncGreeterClient(const std::shared_ptr<grpc::Channel>& channel)
        : stub_(ContainerUpdateHandler::NewStub(channel)) {
        thread_test = std::thread(&AyncGreeterClient::AsyncCompleteRpc, this);
    }

    std::thread *get_thread() {return &thread_test;}

    void updateContainerQuota(uint32_t cgroup_id, uint64_t new_quota, const std::string& change, uint32_t seq_num) {
        ec::rpc::containerUpdate::ContainerQuotaRequest request;
        request.set_cgroupid(int32_t(cgroup_id));
        request.set_newquota(new_quota);
        request.set_resizeflag(change);
        request.set_sequencenum(int32_t(seq_num));

        auto *call = new AsyncClientCall;
        call->response_reader =
                stub_->PrepareAsyncReqQuotaUpdate(&call->context, request, &cq_);
        call->response_reader->StartCall();
        call->response_reader->Finish(&call->reply, &call->status, (void*)call);
    }

    void AsyncCompleteRpc() {
        void* got_tag;
        bool ok = false;

        // Block until the next result is available in the completion queue "cq".
        while (cq_.Next(&got_tag, &ok)) {
            // The tag in this example is the memory location of the call object
            auto* call = static_cast<AsyncClientCall*>(got_tag);

            // Verify that the request was completed successfully. Note that "ok"
            // corresponds solely to the request for updates introduced by Finish().
//            GPR_ASSERT(ok);

            if (call->status.ok())
                std::cout << "rx msg: " << call->reply.cgroupid() << ", " << call->reply.updatequota() << ", "
                    << call->reply.errorcode() << ", " << call->reply.sequencenum() << std::endl;
            else
                std::cout << "RPC failed" << std::endl;

            // Once we're complete, deallocate the call object.
            delete call;
        }
    }



private:
    struct AsyncClientCall {
        // Container for the data we expect from the server.
        ContainerQuotaReply reply;

        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        grpc::ClientContext context;

        // Storage for the status of the RPC upon completion.
        grpc::Status status;

        std::unique_ptr<grpc::ClientAsyncResponseReader<ContainerQuotaReply>> response_reader;
    };

    // Out of the passed in Channel comes the stub, stored here, our view of the
    // server's exposed services.
    std::unique_ptr<ContainerUpdateHandler::Stub> stub_;

    // The producer-consumer queue we use to communicate asynchronously with the
    // gRPC runtime.
    grpc::CompletionQueue cq_;
    std::thread thread_test;
};


int main () {
    AyncGreeterClient greeter(grpc::CreateChannel(
            "192.168.6.7:4448", grpc::InsecureChannelCredentials()));
//    std::thread thread_ = std::thread(&AyncGreeterClient::AsyncCompleteRpc, &greeter);

    for (int i = 0; i < 4; i++) {
        greeter.updateContainerQuota(123, 100000001, "incr", 4);
    }
    greeter.get_thread()->join();
//    thread_.join();
//    auto channel = grpc::CreateChannel("192.168.6.7:4448", grpc::InsecureChannelCredentials());
//    auto stub = ContainerUpdateHandler::NewStub(channel);
//
//    ec::rpc::containerUpdate::ContainerQuotaRequest txMsg;
//    ec::rpc::containerUpdate::ContainerQuotaReply rxMsg;
//    txMsg.set_cgroupid(int32_t(123));
//    txMsg.set_newquota(100000001);
//    txMsg.set_resizeflag("incr");
//    txMsg.set_sequencenum(int32_t(4));
//    grpc::Status status = stub->ReqQuotaUpdate(&context, txMsg, &rxMsg);
//    ec::rpc::ExportPodSpec txMsg;
//    txMsg.set_docker_id("this-be-docker-id");
//    txMsg.set_cgroup_id(123);
//    txMsg.set_node_ip("123.123.12.1");
//    ec::rpc::PodSpecReply rxMsg;
//    grpc::Status status = stub->ReportPodSpec(&context, txMsg, &rxMsg);


//    if(status.ok()) {
//        std::cout << "rx msg: " << rxMsg.cgroupid() << ", " << rxMsg.updatequota() << ", " << rxMsg.errorcode() << ", " << rxMsg.sequencenum() << std::endl;
//    }
//    else {
//        std::cout << "status: " << status.error_message() << std::endl;
//        std::cout << "error code: " << status.error_code() << std::endl;
//        std::cout << "details: " << status.error_details() << std::endl;
//    }
}

//class DeployerExportClient {
//public:
//    DeployerExportClient(std::shared_ptr<grpc::Channel> channel) : stub_(ec::rpc::DeployerExport::NewStub(channel)) {}
//
//private:
//    std::unique_ptr<ec::rpc::DeployerExport::Stub> stub_;
//};