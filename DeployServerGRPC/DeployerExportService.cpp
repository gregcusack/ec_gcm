//
// Created by greg on 4/29/20.
//

#include "DeployerExportService.h"

int ec::rpc::DeployerExportService::insertPodSpec(const ec::rpc::ExportPodSpec *pod) {
    if(!pod) { std::cout << "[ERROR DeployService]: ExportPodSpec *pod is NULL"; return -1; }

    mapLock.lock();
    auto inserted = deployedPods.try_emplace(SubContainer::ContainerId(pod->cgroup_id(),
            om::net::ip4_addr::from_string(pod->node_ip())), pod->docker_id()).second;
    mapLock.unlock();

    assert(inserted); //if !asserted, it means container already has been deployed. shouldn't see this

    return 0;
}

void ec::rpc::DeployerExportService::setPodSpecReply(const ExportPodSpec* pod,
        ec::rpc::PodSpecReply *reply, std::string &status) {
    if(!reply || !pod) {
        std::cout << "[ERROR DeployService]: ExportPodSpec reply or pod is NULL";
        return;
    }

    reply->set_docker_id(pod->docker_id());
    reply->set_cgroup_id(pod->cgroup_id());
    reply->set_node_ip(pod->node_ip());
    reply->set_thanks(status);
}
