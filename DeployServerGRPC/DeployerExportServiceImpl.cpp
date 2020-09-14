//
// Created by greg on 4/29/20.
//

#include "DeployerExportServiceImpl.h"

grpc::Status
ec::rpc::DeployerExportServiceImpl::ReportPodSpec(grpc::ServerContext *context, const ec::rpc::ExportPodSpec *pod,
                                                  ec::rpc::PodSpecReply *reply) {
    std::string status;
    int ret = insertPodSpec(pod);
    //std::cout << "Insert Pod Spec ret: " << ret << std::endl;
    status = ret ? fail : success;
    if(status =="thx") {
        //std::cout << "spinning up matching thread" << std::endl;
        spinUpDockerIdThread(SubContainer::ContainerId(pod->cgroup_id(), pod->node_ip()), pod->docker_id());
    }
    setPodSpecReply(pod, reply, status);
    return grpc::Status::OK;
}

/**
 * 1) Delete from sc_ac_map - Just delete entry. sc_ac_map.remove(sc_id);
 * 2) Delete from subcontainers map. - Delete SubContainer* and then delete entry. subcontainers.remove(sc_id)
 * 3) Delete from deployedPods map. - Just delete entry. deployedPods.remove(sc_id)
 * 4) Reduce Pod Fair share memory and CPU
 * 5) Return to ec_deployer
 */
grpc::Status
ec::rpc::DeployerExportServiceImpl::DeletePod(grpc::ServerContext *context, const ec::rpc::ExportDeletePod *pod,
                                              ec::rpc::DeletePodReply *reply) {

    std::cout << "New Delete Pod Received" << std::endl;
    auto sc_id = getScIdFromDockerId(pod->docker_id());
    if(!sc_id.cgroup_id) {
        std::cout << "[ERROR]: Docker Id to sc_id failed" << std::endl;
        setDeletePodReply(pod, reply, fail);
        return grpc::Status::CANCELLED;
    }
    std::cout << "Sc_id to delete: " << sc_id << std::endl;
    std::string s1, s2, s3, s4, status;

    uint64_t sc_mem_limit = ec->get_subcontainer(sc_id).sc_get_mem_limit_in_pages();
    uint64_t quota = ec->get_subcontainer(sc_id).get_cpu_stats()->get_quota(); //todo: race condition
    std::cout << "deleted container quota: " << quota << std::endl;
    uint64_t mem_alloced_in_pages = 0;
    for (const auto &i : ec->ec_get_subcontainers()) {
        mem_alloced_in_pages += i.second->sc_get_mem_limit_in_pages();
    }
    std::cout << "tot mem in sys pre delete pod: " << mem_alloced_in_pages + ec->get_memory_available() << std::endl;

    s1 = deleteFromScAcMap(sc_id) ? fail : success;
    s2 = deleteFromSubcontainersMap(sc_id) ? fail : success;
    s3 = deleteFromDeployedPodsMap(sc_id) ? fail : success;
    s4 = deleteFromDockerIdScMap(pod->docker_id()) ? fail : success;

    std::cout << "fair cpu share pre delete: " << ec->get_fair_cpu_share() << std::endl;
    std::cout << "pre delete unalloc rt + allcoc_rt: " << ec->get_cpu_unallocated_rt() + ec->get_alloc_rt() << std::endl;
    //CPU
    ec->update_fair_cpu_share();
    ec->decr_alloc_rt(quota);
    ec->incr_unallocated_rt(quota);
    std::cout << "fair cpu share post delete: " << ec->get_fair_cpu_share() << std::endl;
    std::cout << "post delete unalloc rt + allcoc_rt: " << ec->get_cpu_unallocated_rt() + ec->get_alloc_rt() << std::endl;

    //MEM
    std::cout << "delete pod mem_limit to ret to global pool: " << sc_mem_limit << std::endl;
    std::cout << "ec_mem_avail pre delete: " << ec->get_memory_available() << std::endl;
    ec->ec_increment_memory_available_in_pages(sc_mem_limit);
    std::cout << "ec_mem_avail post delete: " << ec->get_memory_available() << std::endl;

    mem_alloced_in_pages = 0;
    for (const auto &i : ec->ec_get_subcontainers()) {
        mem_alloced_in_pages += i.second->sc_get_mem_limit_in_pages();
    }
    std::cout << "tot mem in sys post delete pod: " << mem_alloced_in_pages + ec->get_memory_available() << std::endl;


    status = (s1 != success || s2 != success || s3 != success || s4 != success) ? fail : success;

    setDeletePodReply(pod, reply, status);

    return grpc::Status::OK;
}


grpc::Status
ec::rpc::DeployerExportServiceImpl::ReportAppSpec(grpc::ServerContext *context, const ec::rpc::ExportAppSpec *appSpec,
                                                  ec::rpc::AppSpecReply *reply) {
    // std::cout << "Report App Spec Values: " << std::endl;
    // std::cout << "App Name: " <<  appSpec->app_name() << std::endl;
    // std::cout << "CPU Limit: " << appSpec->cpu_limit()  << std::endl;
    // std::cout << "Mem Limit " << appSpec->mem_limit_in_pages()  << std::endl;

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
    ec->set_unallocated_rt(ec->get_total_cpu());
    // Passed in value is in MiB but ec_resize_memory_max takes in number of pages
    ec->ec_resize_memory_max((appSpec->mem_limit()*1048576)/4000);
    ec->ec_set_memory_available((appSpec->mem_limit()*1048576)/4000);

    std::cout << "Set CPU Limit: " << ec->get_total_cpu()  << std::endl;
    std::cout << "Set Mem Limit " << ec->get_mem_limit()  << std::endl;

    // Set response here
    reply->set_app_name(appSpec->app_name());
    reply->set_cpu_limit(appSpec->cpu_limit());
    reply->set_mem_limit(appSpec->mem_limit());
    reply->set_thanks(success);


    return grpc::Status::OK;
}



int ec::rpc::DeployerExportServiceImpl::insertPodSpec(const ec::rpc::ExportPodSpec *pod) {
    if(!pod) { std::cout << "[ERROR DeployService]: ExportPodSpec *pod is NULL"; return -1; }

    //std::cout << "sc_id to insertPodSpec: " << SubContainer::ContainerId(pod->cgroup_id(), pod->node_ip()) << std::endl;
    std::cout << "sc_id to insertPodSpec: " << SubContainer::ContainerId(pod->cgroup_id(), pod->node_ip()) << std::endl;

    dep_pod_lock.lock();
    auto inserted = deployedPods.emplace(
            SubContainer::ContainerId(pod->cgroup_id(), pod->node_ip()),
            pod->docker_id()
            ).second;
    dep_pod_lock.unlock();

    if(!inserted) {
        std::cout << "[ERROR]: Already deployed pod!" << std::endl;
        return -2;
    }

    std::unique_lock<std::mutex> lk(dockId_sc_lock);
    auto dockInsert = dockerToSubContainer.emplace(
            pod->docker_id(),
            SubContainer::ContainerId(pod->cgroup_id(), pod->node_ip())
            ).second;

    if(!dockInsert) {
        std::cout << "[ERROR]: Docker ID already exists" << std::endl;
        return -3;
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

    //todo: delete this after we match
    auto *args = new matchingThreadArgs(sc_id, docker_id);
    //std::cout << "created matching args: " << args->sc_id << ", " << args->docker_id << std::endl;
    std::thread match_sc_id_thread(&DeployerExportServiceImpl::scIdToDockerIdMatcherThread, this, (void*)args);
    match_sc_id_thread.detach();
//    match_sc_id_thread.join();
}

void ec::rpc::DeployerExportServiceImpl::scIdToDockerIdMatcherThread(void* arguments) {
    auto threadArgs = reinterpret_cast<matchingThreadArgs*>(arguments);
    std::unique_lock<std::mutex> lk(cv_mtx);
    cv.wait(lk, [this, threadArgs] {
            //std::cout << "in cv wait" << std::endl;
        auto itr = ec->get_sc_ac_map_for_update()->find(threadArgs->sc_id);
        //std::cout << "wait for sc_id to exist in sc_ac_map: " << threadArgs->sc_id << ", d_id: " << threadArgs->docker_id << std::endl;
        return itr != ec->get_sc_ac_map_for_update()->end();
    });

    //std::cout << "sc in sc_ac map. set docker_id" << std::endl;
    std::lock_guard<std::mutex> lk_dock(cv_mtx_dock);
    ec->get_subcontainer(threadArgs->sc_id).set_docker_id(threadArgs->docker_id);
    if(unlikely(ec->get_subcontainer(threadArgs->sc_id).get_docker_id().empty())) {
        std::cout << "docker_id set failed in grpcDockerIdMatcher()!" << std::endl;
    } else {
        //std::cout << "docker_id set: " << ec->get_subcontainer(threadArgs->sc_id).get_docker_id() << std::endl;
    }
    cv_dock.notify_one();
    // std::cout << "<-------------------------  spinUpDockerIdThread " << threadArgs->sc_id <<  "END ------------------------->" << std::endl;
    // std::cout << "match thread end" << std::endl;
}

int ec::rpc::DeployerExportServiceImpl::deleteFromScAcMap(const ec::SubContainer::ContainerId &sc_id) {

    std::unique_lock<std::mutex> lk(cv_mtx);
    if(ec->get_sc_ac_map_for_update()->find(sc_id) != ec->get_sc_ac_map_for_update()->end()) {
        ec->get_sc_ac_map_for_update()->erase(sc_id);
    }
    else {
        std::cerr << "[GRPC Service Error]: Can't find sc_id to delete! sc_id: " << sc_id << std::endl;
        return -1;
    }
    return 0;

}

int ec::rpc::DeployerExportServiceImpl::deleteFromSubcontainersMap(const ec::SubContainer::ContainerId &sc_id) {
    std::unique_lock<std::mutex> lk(sc_lock);
// todo: force others functions that access subcontainers map to use this
    return ec->ec_delete_from_subcontainers_map(sc_id);

}

int ec::rpc::DeployerExportServiceImpl::deleteFromDeployedPodsMap(const ec::SubContainer::ContainerId &sc_id) {
    std::unique_lock<std::mutex> lk(dep_pod_lock);
    if(deployedPods.find(sc_id) != deployedPods.end()) {
        deployedPods.erase(sc_id);
    }
    else {
        std::cerr << "[GRPC Service Error]: Can't find sc_id to delete from deployedPods! sc_id: " << sc_id << std::endl;
        return -1;
    }
    return 0;
}

int ec::rpc::DeployerExportServiceImpl::deleteFromDockerIdScMap(const std::string &docker_id) {
    std::unique_lock<std::mutex> lk(dockId_sc_lock);
    if(dockerToSubContainer.find(docker_id) != dockerToSubContainer.end()) {
        dockerToSubContainer.erase(docker_id);
    }
    else {
        std::cerr << "[GRPC Service Error]: Can't find docker ID to delete from deployedPods! sc_id: " << docker_id << std::endl;
        return -1;
    }
    return 0;
}

ec::SubContainer::ContainerId ec::rpc::DeployerExportServiceImpl::getScIdFromDockerId(const std::string &docker_id) {
    std::unique_lock<std::mutex> lk(dockId_sc_lock);
    if(dockerToSubContainer.find(docker_id) != dockerToSubContainer.end()) {
        return dockerToSubContainer.find(docker_id)->second;
    }
    else {
        std::cerr << "[GRPC Service Error]: Can't find sc_id from dockerId!: " << docker_id << std::endl;
        return SubContainer::ContainerId();
    }
}

void ec::rpc::DeployerExportServiceImpl::setDeletePodReply(const ec::rpc::ExportDeletePod *pod,
                                                           ec::rpc::DeletePodReply *reply, const std::string &status) {

    if(!reply || !pod) {
        std::cout << "[ERROR DeployService]: ExportPodSpec reply or pod is NULL";
        return;
    }

    reply->set_docker_id(pod->docker_id());
    reply->set_thanks(status);
}

