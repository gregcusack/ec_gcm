//
// Created by greg on 4/29/20.
//

#include "DeployerExportServiceImpl.h"

grpc::Status
ec::rpc::DeployerExportServiceImpl::ReportPodSpec(grpc::ServerContext *context, const ec::rpc::ExportPodSpec *pod,
                                                  ec::rpc::PodSpecReply *reply) {
    std::string status;
    int ret = insertPodSpec(pod);
    std::cout << "Insert Pod Spec ret: " << ret << std::endl;
    status = ret ? fail : success;
    if(status =="thx") {
        std::cout << "spinning up matching thread" << std::endl;
        spinUpDockerIdThread(SubContainer::ContainerId(pod->cgroup_id(), pod->node_ip()), pod->docker_id());
    }
    setPodSpecReply(pod, reply, status);
    return grpc::Status::OK;
}

int ec::rpc::DeployerExportServiceImpl::insertPodSpec(const ec::rpc::ExportPodSpec *pod) {
    if(!pod) { std::cout << "[ERROR DeployService]: ExportPodSpec *pod is NULL"; return -1; }

    mapLock.lock();
    auto inserted = deployedPods.emplace(
            SubContainer::ContainerId(pod->cgroup_id(), pod->node_ip()),
            pod->docker_id()
            ).second;
    mapLock.unlock();

    if(!inserted) {
        std::cout << "[ERROR]: Already deployed pod!" << std::endl;
        return -2;
    }


    return 0;
}

void ec::rpc::DeployerExportServiceImpl::setPodSpecReply(const ExportPodSpec* pod,
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

void ec::rpc::DeployerExportServiceImpl::spinUpDockerIdThread(const ec::SubContainer::ContainerId &sc_id,
                                                              const std::string &docker_id) {

    auto *args = new matchingThreadArgs(sc_id, docker_id);
    std::cout << "created matching args: " << args->sc_id << ", " << args->docker_id << std::endl;
    std::thread match_sc_id_thread(&DeployerExportServiceImpl::scIdToDockerIdMatcherThread, this, (void*)args);
    match_sc_id_thread.detach();
//    match_sc_id_thread.join();
}

void ec::rpc::DeployerExportServiceImpl::scIdToDockerIdMatcherThread(void* arguments) {
    auto threadArgs = reinterpret_cast<matchingThreadArgs*>(arguments);
    std::unique_lock<std::mutex> lk(cv_mtx);
    cv.wait(lk, [this, threadArgs] {
            std::cout << "in cv wait" << std::endl;
        auto itr = ec->get_sc_ac_map_for_update()->find(threadArgs->sc_id);
        std::cout << "wait for sc_id to exist in sc_ac_map: " << threadArgs->sc_id << ", d_id: " << threadArgs->docker_id << std::endl;
        return itr != ec->get_sc_ac_map_for_update()->end();
    });

    std::cout << "sc in sc_ac map. set docker_id" << std::endl;
    ec->get_subcontainer(threadArgs->sc_id).set_docker_id(threadArgs->docker_id);
    if(unlikely(ec->get_subcontainer(threadArgs->sc_id).get_docker_id().empty())) {
        std::cout << "docker_id set failed in grpcDockerIdMatcher()!" << std::endl;
    } else {
        std::cout << "docker_id set: " << ec->get_subcontainer(threadArgs->sc_id).get_docker_id() << std::endl;
    }
    // std::cout << "<-------------------------  spinUpDockerIdThread " << threadArgs->sc_id <<  "END ------------------------->" << std::endl;

}

grpc::Status
ec::rpc::DeployerExportServiceImpl::ReportAppSpec(grpc::ServerContext *context, const ec::rpc::ExportAppSpec *appSpec,
                                                  ec::rpc::AppSpecReply *reply) {
    // std::cout << "Report App Spec Values: " << std::endl;
    // std::cout << "App Name: " <<  appSpec->app_name() << std::endl;
    // std::cout << "CPU Limit: " << appSpec->cpu_limit()  << std::endl;
    // std::cout << "Mem Limit " << appSpec->mem_limit()  << std::endl;

    if(!reply || !appSpec) {
        std::cout << "[ERROR DeployService]: ReportAppSpec reply or appSpec is NULL";
        return grpc::Status::CANCELLED;
    }

    // Set Application Global limits here:
    if (!ec){
        std::cout << "[ERROR DeployService]: EC is NULL in ReportAppSpec";
        return grpc::Status::CANCELLED;
    }
    // Passed in value is in mi but set_total_cpu takes ns 
    ec->set_total_cpu(appSpec->cpu_limit()*100000);
    // Passed in value is in MiB but ec_resize_memory_max takes in number of pages
    ec->ec_resize_memory_max((appSpec->mem_limit()*1048576)/4000);

    std::cout << "Set CPU Limit: " << ec->get_total_cpu()  << std::endl;
    std::cout << "Set Mem Limit " << ec->get_mem_limit()  << std::endl;

    // Set response here
    reply->set_app_name(appSpec->app_name());
    reply->set_cpu_limit(appSpec->cpu_limit());
    reply->set_mem_limit(appSpec->mem_limit());
    reply->set_thanks(success);


    return grpc::Status::OK;
}


