//
// Created by greg on 4/30/20.
//

#include <grpc++/grpc++.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include "../DeployServerGRPC/deploy.grpc.pb.h"
#include "../DeployServerGRPC/DeployerExportServiceImpl.h"
#include "../Agents/AgentClientDB.h"

ec::AgentClientDB* ec::AgentClientDB::agent_clients_db_instance = nullptr;

int main () {
    auto channel = grpc::CreateChannel("10.0.2.15:4447", grpc::InsecureChannelCredentials());
    auto stub = ec::rpc::DeployerExport::NewStub(channel);
    grpc::ClientContext context;
    ec::rpc::ExportPodSpec txMsg;
    txMsg.set_docker_id("this-be-docker-id");
    txMsg.set_cgroup_id(123);
    txMsg.set_node_ip("123.123.12.1");
    ec::rpc::PodSpecReply rxMsg;
    grpc::Status status = stub->ReportPodSpec(&context, txMsg, &rxMsg);

    if(status.ok()) {
        std::cout << "rx msg: " << rxMsg.docker_id() << ", " << rxMsg.cgroup_id() << ", " << rxMsg.node_ip() << ", " << rxMsg.thanks() << std::endl;
    }
    else {
        std::cout << "status: " << status.error_message() << std::endl;
        std::cout << "error code: " << status.error_code() << std::endl;
        std::cout << "details: " << status.error_details() << std::endl;
    }
}

//class DeployerExportClient {
//public:
//    DeployerExportClient(std::shared_ptr<grpc::Channel> channel) : stub_(ec::rpc::DeployerExport::NewStub(channel)) {}
//
//private:
//    std::unique_ptr<ec::rpc::DeployerExport::Stub> stub_;
//};