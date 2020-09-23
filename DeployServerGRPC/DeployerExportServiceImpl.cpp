//target_compile_definitions(ec_gcm PUBLIC DEBUG_MAX=1)
// Created by greg on 4/29/20.
//

#include "DeployerExportServiceImpl.h"

grpc::Status
ec::rpc::DeployerExportServiceImpl::ReportPodSpec(grpc::ServerContext *context, const ec::rpc::ExportPodSpec *pod,
                                                  ec::rpc::PodSpecReply *reply) {
    std::string status;
    int ret = insertPodSpec(pod);
    SPDLOG_DEBUG("Insert Pod Spec ret: {}", ret);

    status = ret ? fail : success;
    if(status =="thx") {
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

    SPDLOG_INFO("New Delete Pod Received");
    auto sc_id = getScIdFromDockerId(pod->docker_id());
    if(!sc_id.cgroup_id) {
        SPDLOG_ERROR("[ERROR]: Docker Id to sc_id failed");
        setDeletePodReply(pod, reply, fail);
        return grpc::Status::CANCELLED;
    }
    SPDLOG_DEBUG("Sc_id to delete: {}", sc_id);
    std::string s1, s2, s3, s4, status;

    uint64_t sc_mem_limit = ec->get_subcontainer(sc_id).get_mem_limit_in_pages();
    uint64_t quota = ec->get_subcontainer(sc_id).get_cpu_stats()->get_quota(); //todo: race condition

#if(SPDLOG_ACTIVE_LEVEL == SPDLOG_LEVEL_TRACE)
    SPDLOG_TRACE("deleted container quota, mem_in_pages: {}, {}", quota, sc_mem_limit);
    uint64_t mem_alloced_in_pages = 0;
    for (const auto &i : ec->get_subcontainers()) {
        mem_alloced_in_pages += i.second->get_mem_limit_in_pages();
    }
    SPDLOG_TRACE("tot mem in sys pre delete pod: {}", mem_alloced_in_pages + ec->get_unallocated_memory_in_pages());
    SPDLOG_TRACE("tot alloc, unalloc mem pre delete pod: {}, {}", ec->get_allocated_memory_in_pages(), ec->get_unallocated_memory_in_pages());
    SPDLOG_TRACE("tot mem in sys (alloc+unalloc) pre delete: {}", ec->get_allocated_memory_in_pages() + ec->get_unallocated_memory_in_pages());
    SPDLOG_TRACE("tot_alloc virtual, tot_alloc physical pre delete: {}, {}", ec->get_allocated_memory_in_pages(), ec->get_tot_mem_alloc_in_pages());
    SPDLOG_TRACE("fair cpu share pre delete: {}", ec->get_fair_cpu_share());
    SPDLOG_TRACE("pre delete unalloc rt + allcoc_rt: {}", ec->get_cpu_unallocated_rt() + ec->get_alloc_rt());
#endif

    s1 = deleteFromScAcMap(sc_id) ? fail : success;
    s2 = deleteFromSubcontainersMap(sc_id) ? fail : success;
    s3 = deleteFromDeployedPodsMap(sc_id) ? fail : success;
    s4 = deleteFromDockerIdScMap(pod->docker_id()) ? fail : success;

    //CPU
    ec->update_fair_cpu_share();
    ec->decr_alloc_rt(quota);
    ec->incr_unallocated_rt(quota);

#if(SPDLOG_ACTIVE_LEVEL == SPDLOG_LEVEL_TRACE)
    SPDLOG_TRACE("fair cpu share post delete: {}", ec->get_fair_cpu_share());
    SPDLOG_TRACE("post delete unalloc rt + allcoc_rt: {}", ec->get_cpu_unallocated_rt() + ec->get_alloc_rt());
    SPDLOG_TRACE("delete pod mem_limit to ret to global pool: {}", sc_mem_limit);
    SPDLOG_TRACE("ec_mem_avail pre delete: {}", ec->get_unallocated_memory_in_pages());
#endif

    //MEM
    ec->incr_unalloc_memory_in_pages(sc_mem_limit);
    ec->decr_alloc_memory_in_pages(sc_mem_limit);

#if(SPDLOG_ACTIVE_LEVEL == SPDLOG_LEVEL_TRACE)
    SPDLOG_TRACE("ec_mem_avail post delete: {}", ec->get_unallocated_memory_in_pages());

    mem_alloced_in_pages = 0;
    for (const auto &i : ec->get_subcontainers()) {
        mem_alloced_in_pages += i.second->get_mem_limit_in_pages();
    }
    SPDLOG_TRACE("tot mem in sys post delete pod: {}", mem_alloced_in_pages + ec->get_unallocated_memory_in_pages());
    SPDLOG_TRACE("tot alloc, unalloc mem post delete pod: {}, {}", ec->get_allocated_memory_in_pages(), ec->get_unallocated_memory_in_pages());
    SPDLOG_TRACE("tot mem in sys (alloc+unalloc) post delete: {}", ec->get_allocated_memory_in_pages() + ec->get_unallocated_memory_in_pages());
    SPDLOG_TRACE("tot_alloc virtual, tot_alloc physical post delete: {}, {}", ec->get_allocated_memory_in_pages(), ec->get_tot_mem_alloc_in_pages());
#endif

    status = (s1 != success || s2 != success || s3 != success || s4 != success) ? fail : success;

    setDeletePodReply(pod, reply, status);

    SPDLOG_DEBUG("delete pod completed with status: {}", status);
    return grpc::Status::OK;
}


grpc::Status
ec::rpc::DeployerExportServiceImpl::ReportAppSpec(grpc::ServerContext *context, const ec::rpc::ExportAppSpec *appSpec,
                                                  ec::rpc::AppSpecReply *reply) {
    if(!reply || !appSpec) {
        SPDLOG_ERROR("ReportAppSpec reply or appSpec is NULL");
        return grpc::Status::CANCELLED;
    }

    // Set Application Global limits here:
    if (!ec){
        SPDLOG_ERROR("EC is NULL in ReportAppSpec");
        return grpc::Status::CANCELLED;
    }
    // Passed in value is in mi but set_total_cpu takes ns
    ec->set_total_cpu(appSpec->cpu_limit()*100000);
    ec->set_unallocated_rt(ec->get_total_cpu());
    // Passed in value is in MiB but set_memory_limit_in_pages takes in number of pages
    ec->set_memory_limit_in_pages((appSpec->mem_limit() * 1048576) / 4000);
    ec->set_unalloc_memory_in_pages((appSpec->mem_limit() * 1048576) / 4000);
    ec->set_alloc_memory_in_pages(0);

    SPDLOG_DEBUG("Set CPU Limit: {}", ec->get_total_cpu());
    SPDLOG_DEBUG("Set Mem Limit {}", ec->get_mem_limit_in_pages());

    // Set response here
    reply->set_app_name(appSpec->app_name());
    reply->set_cpu_limit(appSpec->cpu_limit());
    reply->set_mem_limit(appSpec->mem_limit());
    reply->set_thanks(success);

    return grpc::Status::OK;
}



int ec::rpc::DeployerExportServiceImpl::insertPodSpec(const ec::rpc::ExportPodSpec *pod) {
    if(!pod) { SPDLOG_ERROR("ExportPodSpec *pod is NULL"); }

    SPDLOG_DEBUG("sc_id to insertPodSpec: {}", SubContainer::ContainerId(pod->cgroup_id(), pod->node_ip()));

    dep_pod_lock.lock();
    auto inserted = deployedPods.emplace(
            SubContainer::ContainerId(pod->cgroup_id(), pod->node_ip()),
            pod->docker_id()
            ).second;
    dep_pod_lock.unlock();

    if(!inserted) {
        SPDLOG_ERROR("Already deployed pod!");
        return -2;
    }

    std::unique_lock<std::mutex> lk(dockId_sc_lock);
    auto dockInsert = dockerToSubContainer.emplace(
            pod->docker_id(),
            SubContainer::ContainerId(pod->cgroup_id(), pod->node_ip())
            ).second;

    if(!dockInsert) {
        SPDLOG_ERROR("Docker ID already exists");
        return -3;
    }

    return 0;
}

void ec::rpc::DeployerExportServiceImpl::setPodSpecReply(const ExportPodSpec* pod,
                                                         ec::rpc::PodSpecReply *reply, std::string &status) {
    if(!reply || !pod) {
        SPDLOG_ERROR("ExportPodSpec reply or pod is NULL");
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
    std::thread match_sc_id_thread(&DeployerExportServiceImpl::scIdToDockerIdMatcherThread, this, (void*)args);
    match_sc_id_thread.detach();
}

void ec::rpc::DeployerExportServiceImpl::scIdToDockerIdMatcherThread(void* arguments) {
    auto threadArgs = reinterpret_cast<matchingThreadArgs*>(arguments);
    std::unique_lock<std::mutex> lk(cv_mtx);
    cv.wait(lk, [this, threadArgs] {
        auto itr = ec->get_sc_ac_map_for_update()->find(threadArgs->sc_id);
        SPDLOG_TRACE("wait for sc_id to exist in sc_ac_map: {}, d_id: {}", threadArgs->sc_id, threadArgs->docker_id);
        return itr != ec->get_sc_ac_map_for_update()->end();
    });

    std::lock_guard<std::mutex> lk_dock(cv_mtx_dock);
    ec->get_subcontainer(threadArgs->sc_id).set_docker_id(threadArgs->docker_id);
    if(unlikely(ec->get_subcontainer(threadArgs->sc_id).get_docker_id().empty())) {
        SPDLOG_ERROR("docker_id set failed in grpcDockerIdMatcher()!");
    }
    cv_dock.notify_one();
}

int ec::rpc::DeployerExportServiceImpl::deleteFromScAcMap(const ec::SubContainer::ContainerId &sc_id) {

    std::unique_lock<std::mutex> lk(cv_mtx);
    if(ec->get_sc_ac_map_for_update()->find(sc_id) != ec->get_sc_ac_map_for_update()->end()) {
        ec->get_sc_ac_map_for_update()->erase(sc_id);
    }
    else {
        SPDLOG_ERROR("Can't find sc_id to delete! sc_id: {}", sc_id);
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
        SPDLOG_ERROR("Can't find sc_id to delete from deployedPods! sc_id: {}", sc_id);
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
        SPDLOG_ERROR("Can't find docker ID to delete from deployedPods! sc_id: {}", docker_id);
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
        SPDLOG_ERROR("Can't find sc_id from dockerId!: {}", docker_id);
        return SubContainer::ContainerId();
    }
}

void ec::rpc::DeployerExportServiceImpl::setDeletePodReply(const ec::rpc::ExportDeletePod *pod,
                                                           ec::rpc::DeletePodReply *reply, const std::string &status) {

    if(!reply || !pod) {
        SPDLOG_ERROR("ExportPodSpec reply or pod is NULL");
        return;
    }

    reply->set_docker_id(pod->docker_id());
    reply->set_thanks(status);
}