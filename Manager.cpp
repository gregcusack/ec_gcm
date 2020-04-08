//
// Created by Greg Cusack on 11/5/19.
//

#include "Manager.h"
#include "Agents/AgentClientDB.h"

ec::Manager::Manager( uint32_t server_counts, ec::ip4_addr gcm_ip, uint16_t server_port, std::vector<Agent *> &agents )
            : Server(server_counts, gcm_ip, server_port, agents), seq_number(0) {

    //init server
    initialize();
    //TODO: this is temporary. should be fixed. there is no need to have 2 instance of agentClients
//    AgentClientDB* acdb = acdb->get_agent_client_db_instance();
}

void ec::Manager::start(const std::string &app_name, const std::vector<std::string> &app_images, const std::vector<std::string> &pod_names,  const std::string &gcm_ip) {
    //A thread to listen for subcontainers' events
    std::thread event_handler_thread(&ec::Server::serve, this);
    std::thread application_deployment_thread(&ec::ECAPI::create_ec, this, app_name, app_images, pod_names, gcm_ip);
    // //Another thread to run a management application
    application_deployment_thread.join();
    sleep(10);
    std::cerr<<"[dbg] manager::just before running the app thread\n";
    std::thread application_thread(&ec::Manager::run, this);
    application_thread.join();
    event_handler_thread.join();
}

int ec::Manager::handle_cpu_usage_report(const ec::msg_t *req, ec::msg_t *res) {
    if (req == nullptr || res == nullptr) {
        std::cout << "req or res == null in handle_cpu_usage_report()" << std::endl;
        exit(EXIT_FAILURE);
    }
//    std::mutex cpulock;
    if (req->req_type != _CPU_) { return __ALLOC_FAILED__; }

//    auto t1 = std::chrono::high_resolution_clock::now();
    cpulock.lock();
//    std::cout << "------------------------------" << std::endl;
//    std::cout << "cg id: " << req->cgroup_id << std::endl;
    auto sc_id = SubContainer::ContainerId(req->cgroup_id, req->client_ip);
    auto sc = ec_get_sc_for_update(sc_id);
    if (!sc) {
        std::cout << "ERROR! sc is NULL!" << std::endl;
        return __ALLOC_SUCCESS__;
    }
    sc->incr_counter();
    auto req_count = sc->get_counter();
    auto rx_quota = req->rsrc_amnt;
    auto rt_remaining = req->runtime_remaining;
    auto throttled = req->request;
    uint64_t updated_quota = rx_quota;
    uint64_t to_add = 0;
    int ret;
    uint64_t rx_buff;
    double thr_mean = 0;
    uint64_t rt_mean = 0;
    uint64_t total_rt = 0;
    uint32_t seq_num = seq_number;

    if(rx_quota / 1000 != sc->sc_get_quota() / 1000) {
//        std::cout << "quotas do not match (rx, sc->get): (" << rx_quota << ", " << sc->sc_get_quota() << ")" << std::endl;
        cpulock.unlock();
        return __ALLOC_SUCCESS__;
    }

    for (const auto &i : get_subcontainers()) {
        total_rt += i.second->sc_get_quota();
    }
//    std::cout << "total rt given to containers: " << total_rt << std::endl;
    total_rt += ec_get_cpu_unallocated_rt();
    auto tot_rt_and_overrun = total_rt + ec_get_overrun();
    std::cout << "total rt in system: " << total_rt << std::endl;
    std::cout << "total rt + overrun in system: " << tot_rt_and_overrun << std::endl;

    rt_mean = sc->get_cpu_stats()->insert_rt_stats(rt_remaining);
    thr_mean = sc->get_cpu_stats()->insert_th_stats(throttled);

    std::cout << "cpu_unalloc: " << ec_get_cpu_unallocated_rt() << std::endl;

    if(ec_get_overrun() > 0 && rx_quota > ec_get_fair_cpu_share()) {
//        std::cout << "overrun. sc: " << *sc->get_c_id() << std::endl;
        uint64_t to_sub;
        uint64_t amnt_share_over = rx_quota - ec_get_fair_cpu_share();
        uint64_t overrun = ec_get_overrun();
        double percent_over = ((double)rx_quota - (double)ec_get_fair_cpu_share()) / (double)ec_get_fair_cpu_share();
//        std::cout << "percent over: " << percent_over << std::endl;
        if(percent_over > 1) {
            to_sub = (uint64_t) (percent_over * (double) ec_get_cpu_slice());
        }
        else {
            to_sub = (uint64_t) (percent_over * (double)amnt_share_over);
        }
        //TODO: thr_mean is probably important. Take less from containers that are constantly being throttled
//        uint64_t to_sub_frac = (1 - thr_mean) * amnt_share_over;
        if(to_sub < ec_get_cpu_slice() / 2) { //ensures we eventually converge
            to_sub = amnt_share_over;
        }
        to_sub = std::min(overrun, to_sub);
        updated_quota = rx_quota - to_sub;
        ret = set_sc_quota(sc, updated_quota, seq_num);
        if(ret) {
            std::cout << "[ERROR]: GCM. Can't read from socket to resize quota (overrun sub quota). ret: " << ret << std::endl;
        }
        else {
            sc->set_quota_flag(true);
//            std::cout << "successfully resized quota to (overrun): " << updated_quota << "!" << std::endl;
            sc->get_cpu_stats()->flush();
            ec_decr_overrun(to_sub);
            sc->sc_set_quota(updated_quota);
        }
//        sc->get_cpu_stats()->flush();
//        ec_decr_overrun(to_sub);
//        sc->sc_set_quota(updated_quota);
    }
    else if(rx_quota < ec_get_fair_cpu_share() && thr_mean > 0.5) {   //throttled but don't have fair share
//        std::cout << "throt and less than fair share. sc: " << *sc->get_c_id() << std::endl;
        uint64_t amnt_share_lacking = ec_get_fair_cpu_share() - rx_quota;
//        std::cout << "amnt_share_lacking: " << amnt_share_lacking << std::endl;
        if (ec_get_cpu_unallocated_rt() > 0) {
            //TODO: take min of to_Add and slice. don't full reset
//            std::cout << "give back some unalloc_Rt. sc: " << *sc->get_c_id() << std::endl;
            double percent_under = ((double)ec_get_fair_cpu_share() - (double)rx_quota) / (double)ec_get_fair_cpu_share();
//            std::cout << "percent under1: " << percent_under << std::endl;
            if(amnt_share_lacking > ec_get_cpu_slice() / 2) {   //ensure we eventually converge
                to_add = std::min(ec_get_cpu_unallocated_rt(), (uint64_t)(percent_under * (double)amnt_share_lacking));
            }
            else {
                to_add = amnt_share_lacking;
            }
//            to_add = std::min(ec_get_cpu_unallocated_rt(), (uint64_t)(thr_mean * amnt_share_lacking));
//            std::cout << "to_Add: " << to_add << std::endl;
            updated_quota = rx_quota + to_add;
            ret = set_sc_quota(sc, updated_quota, seq_num);
            if(ret) {
                std::cout << "[ERROR]: GCM. Can't read from socket to resize quota (incr fair share). ret: " << ret << std::endl;
            }
            else {
                sc->set_quota_flag(true);
//                std::cout << "successfully resized quota to (fair share 1): " << updated_quota << "!" << std::endl;
                ec_decr_unallocated_rt(to_add);
                sc->sc_set_quota(updated_quota);
                sc->get_cpu_stats()->flush();
            }
//            ec_decr_unallocated_rt(to_add);
//            sc->sc_set_quota(updated_quota);
//            std::cout << "new_quota: " << updated_quota << std::endl;
//            std::cout << "ec_get_unalloc_rt: " << ec_get_cpu_unallocated_rt() << std::endl;
        }
        else { //not enough in unalloc_rt to get back to fair share, even out
//            std::cout << "not enough in unalloc rt. give back slice or overrun.. sc: " << *sc->get_c_id() << std::endl;
            uint64_t overrun;
            double percent_under = ((double)ec_get_fair_cpu_share() - (double)rx_quota) / (double)ec_get_fair_cpu_share();
//            std::cout << "percent under2: " << percent_under << std::endl;
//            overrun = (uint64_t)thr_mean * amnt_share_lacking;
            if(amnt_share_lacking > ec_get_cpu_slice() / 2) { //ensure we eventualyl converge
                overrun = (uint64_t)((double) percent_under * amnt_share_lacking);
            }
            else {
                overrun = amnt_share_lacking;
            }
            updated_quota = rx_quota + overrun;
            ret = set_sc_quota(sc, updated_quota, seq_num);
            if(ret) {
                std::cout << "[ERROR]: GCM. Can't read from socket to resize quota (incr fair share overrun). ret: " << ret << std::endl;
            }
            else {
                sc->set_quota_flag(true);
//                std::cout << "successfully resized quota to (fair share 2): " << updated_quota << "!" << std::endl;
                ec_incr_overrun(overrun);
                sc->sc_set_quota(updated_quota);
                sc->get_cpu_stats()->flush();
            }
//            ec_incr_overrun(overrun);
//            sc->sc_set_quota(updated_quota);
        }
//        sc->get_cpu_stats()->flush();
    }
    else if(thr_mean >= 0.2 && ec_get_cpu_unallocated_rt() > 0) {  //sc_quota > fair share and container got throttled during the last period. need rt
//        std::cout << "throttle. try get alloc. sc:  " << *sc->get_c_id() << std::endl;
        auto extra_rt = std::min(ec_get_cpu_unallocated_rt(), (uint64_t)(2 * thr_mean * ec_get_cpu_slice()));
//        std::cout << "extra_rt: " << extra_rt << std::endl;
        if(extra_rt > 0) {
            updated_quota = rx_quota + extra_rt;
            ret = set_sc_quota(sc, updated_quota, seq_num);
            if(ret) {
                std::cout << "[ERROR]: GCM. Can't read from socket to resize quota (incr). ret: " << ret << std::endl;
            }
            else {
                sc->set_quota_flag(true);
//                std::cout << "successfully resized quota to (incr): " << rx_quota + extra_rt << "!" << std::endl;
                ec_decr_unallocated_rt(extra_rt);
                sc->sc_set_quota(updated_quota);
                sc->get_cpu_stats()->flush();
            }
        }
//        else {
//            std::cout << "extra_rt == 0: " << extra_rt << std::endl;
//        }
//        sc->sc_set_quota(updated_quota);
//        sc->get_cpu_stats()->flush();
    }
    else if(rt_mean > rx_quota * 0.2) { //greater than 20% of quota unused
//        std::cout << "rt_mean > 20% of quota. sc: " << *sc->get_c_id() << std::endl;
        uint64_t new_quota = rx_quota * (1 - 0.2); //sc_quota - sc_rt_remaining + ec_get_cpu_slice();
        new_quota = std::max(ec_get_cpu_slice(), new_quota);
        if(new_quota != rx_quota) {
//            std::cout << "new, old, rt_remain: (" << new_quota << "," << rx_quota << "," << rt_mean << ")" << std::endl;
            ret = set_sc_quota(sc, new_quota, seq_num); //give back what was used + 5ms
            if(ret) {
                std::cout << "[ERROR]: GCM. Can't read from socket to resize quota (decr). ret: " << ret << std::endl;
            }
            else {
                sc->set_quota_flag(true);
//                std::cout << "successfully resized quota to (decr): " << new_quota << "!" << std::endl;

//            std::cout << "old quota, new quota: (" << rx_quota << ", " << new_quota << ")" << std::endl;
                ec_incr_unallocated_rt(rx_quota - new_quota); //unalloc_rt <-- old quota - new quota
                sc->sc_set_quota(new_quota);
                sc->get_cpu_stats()->flush();
            }
        }
//        else {
//            std::cout << "new_quota == old_quota: " << new_quota << std::endl;
//        }
    }
//    else {
//        std::cout << "DO NOTHING" << std::endl;
//    }

    seq_number++;
    cpulock.unlock();
    return __ALLOC_SUCCESS__;

}

int ec::Manager::handle_mem_req(const ec::msg_t *req, ec::msg_t *res, int clifd) {
    if(req == nullptr || res == nullptr) {
        std::cout << "req or res == null in handle_mem_req()" << std::endl;
        exit(EXIT_FAILURE);
    }
    uint64_t ret = 0;
    if(req->req_type != _MEM_) { return __ALLOC_FAILED__; }
    memlock.lock();
    int64_t memory_available = ec_get_memory_available();
    if(memory_available > 0) {
        std::cout << "gcm memory avail > 0. Returning global slice. mem avail: " << memory_available << std::endl;

    }
    else if((memory_available = ec_set_memory_available(handle_reclaim_memory(clifd))) > 0){          //TODO: integrate give back here
        std::cout << "reclaimed mem from other pods. mem rx: " << memory_available << std::endl;
    }
    else {
        memlock.unlock();
        std::cout << "no memory available!" << std::endl;
        res->rsrc_amnt = 0;
        return __ALLOC_FAILED__;
    }

    std::cout << "Handle mem req: success. memory available: " << memory_available << std::endl;
    ret = memory_available > ec_get_memory_slice() ? ec_get_memory_slice() : memory_available;

    std::cout << "mem amnt to ret: " << ret << std::endl;

    ec_decrement_memory_available(ret);
//        memory_available -= ret;

    std::cout << "successfully decrease remaining mem to: " << ec_get_memory_available() << std::endl;

    res->rsrc_amnt = req->rsrc_amnt + ret;   //give back "ret" pages
//    auto sc_id = SubContainer::ContainerId(req->cgroup_id, req->client_ip);
//    res->rsrc_amnt = resize_memory_limit_in_bytes(sc_id, res->rsrc_amnt);


    memlock.unlock();
    res->request = 0;       //give back
    return __ALLOC_SUCCESS__;
}

uint64_t ec::Manager::handle_reclaim_memory(int client_fd) {
    int j = 0;
    char buffer[__BUFF_SIZE__] = {0};
    uint64_t total_reclaimed = 0;
    uint64_t reclaimed = 0;
    uint64_t rx_buff;
    int ret;
    AgentClientDB* acdb = AgentClientDB::get_agent_client_db_instance();
    std::cout << "[INFO] GCM: Trying to reclaim memory from other cgroups!" << std::endl;
    for (const auto &container : get_subcontainers()) {
        if (container.second->get_fd() == client_fd) {
            continue;
        }
        auto ip = container.second->get_c_id()->server_ip;
        std::cout << "ac.size(): " << acdb->get_agent_clients_db_size() << std::endl;
        //for (const auto &agentClient : get_agent_clients()) {
            const auto* target_agent = acdb->get_agent_client_by_ip(ip);
            std::cout << "(agentClient->ip, container ip): (" << target_agent->get_agent_ip() << ", " << ip << ")" << std::endl;
            if (target_agent) {
                auto *reclaim_req = new reclaim_msg;
                reclaim_req->cgroup_id = container.second->get_c_id()->cgroup_id;
                reclaim_req->is_mem = 1;
                //TODO: anyway to get the server to do this?
                if (write(target_agent->get_socket(), (char *) reclaim_req, sizeof(*reclaim_req)) < 0) {
                    std::cout << "[ERROR]: GCM EC Manager id: " << get_manager_id() << ". Failed writing to agent_clients socket"
                              << std::endl;
                }
                ret = read(target_agent->get_socket(), buffer, sizeof(buffer));
                if (ret <= 0) {
                    std::cout << "[ERROR]: GCM. Can't read from socke to reclaim memory" << std::endl;
                }
                rx_buff = *((uint64_t *) buffer);
                reclaimed += rx_buff;
                std::cout << "[INFO] GCM: reclaimed: " << rx_buff << " bytes" << std::endl;
                std::cout << "[INFO] GCM: Current amount of reclaimed memory: " << reclaimed << std::endl;
            }
        }
    std::cout << "[dbg] Recalimed memory at the end of the reclaim function: " << total_reclaimed << std::endl;
    return total_reclaimed;
}

int ec::Manager::handle_req(const msg_t *req, msg_t *res, uint32_t host_ip, int clifd){
    if(req == nullptr || res == nullptr) {
        std::cout << "req or res == null in handle_req()" << std::endl;
        exit(EXIT_FAILURE);
    }
    uint64_t ret = __FAILED__;

    switch(req -> req_type) {
        case _MEM_:
            ret = handle_mem_req(req, res, clifd);
            break;
        case _CPU_:
            ret = handle_cpu_usage_report(req, res);
            break;
        case _INIT_:
            ret = handle_add_cgroup_to_ec(req, res, host_ip, clifd);
            break;
        default:
            std::cout << "[Error]: ECAPI: " << manager_id << ". Handling memory/cpu request failed!" << std::endl;
    }
    return ret;
}

//TODO: this should be separated out into own file
void ec::Manager::run() {
    //ec::SubContainer::ContainerId x ;
    std::cout << "[dbg] In Manager Run function" << std::endl;
    std::cout << "EC Map Size: " << _ec->get_subcontainers().size() << std::endl;
    while(true){
        for(auto sc_ : _ec->get_subcontainers()){
            std::cout << "=================================================================================================\n";
            std::cout << "[READ API]: the memory limit and max_usage in bytes of the container with cgroup id: " << sc_.second->get_c_id()->cgroup_id << std::endl;
            std::cout << " on the node with ip address: " << sc_.first.server_ip  << " is: " << get_memory_limit_in_bytes(sc_.first) << "---" << get_memory_usage_in_bytes(sc_.first) << std::endl;
            std::cout << "[READ API]: machine free: " << get_machine_free_memory(sc_.first) << std::endl;
            std::cout << "=================================================================================================\n";
//            std::cout << "quota is: " << get_cpu_quota_in_us(sc_.first) << "###" << std::endl;
            sleep(1);
        }
        sleep(1);
    }
}




