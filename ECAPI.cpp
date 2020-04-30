//
// Created by greg on 9/12/19.
//

#include "ECAPI.h"
#include "Agents/AgentClientDB.h"

//ec::ECAPI::ECAPI(uint32_t _ec_id)
//    : manager_id(_ec_id) {
 //   // _ec = new ElasticContainer(manager_id, agent_clients);
//}

int ec::ECAPI::create_ec(const std::string &app_name, const std::vector<std::string> &app_images, const std::vector<std::string> &pod_names, const std::string &gcm_ip) {
    _ec = new ElasticContainer(manager_id);

    /* This is the highest level of abstraction provided to the end application developer. 
    Steps to create and deploy the "distributed container":
        1. Create a Pod deployment strategy
        2. Communicate with K8 REST API to deploy the pod on all nodes (based on default kube-scheduler)
        3. Send a request to the specific agent on a node to call sys_connect
    */

    // Step 1: Create a Pod on each of the nodes running an agent
    AgentClientDB* acdb = AgentClientDB::get_agent_client_db_instance();

    int pod_creation;
    int res;
    std::string response;
    std::vector<std::string> node_names;
    std::vector<std::string> node_ips;

    std::vector<SubContainer::ContainerId> scs_per_agent;
    std::vector<SubContainer::ContainerId> scs_done;

//    ec::Facade::DeployFacade::Deploy deployment;
    uint32_t podNameIndex = 0;
    for (const auto &app_image: app_images) {
        std::cout << "app image: " << app_image << std::endl;
        // Step 1: Create a Pod on each of the nodes running an agent - k8s takes care of this
        //         todo: this will change we implement k8-yaml support (still brainstorming how best to do that)           
        std::cout << "[DEPLOY LOG] Generating JSON POD File for image: " << app_image << std::endl;
        //TODO: need to add pod_name config so it only contains alphanumeric characters
        std::string pod_name = pod_names[podNameIndex];
        ec::Facade::JSONFacade::json::createJSONPodDef(app_name, app_image, pod_name, response);
        podNameIndex++;
        // Step 2
        std::cout << "[DEPLOY LOG] Deploying Container With Image: " << app_image << std::endl;
        pod_creation = ec::Facade::DeployFacade::Deploy::deployContainers(response);
        if (pod_creation != 0) {
            std::cout << "[DEPLOY ERROR]:  Error in deploying Container with image: "<< app_image <<".. Exiting" << std::endl;
            return -1;
        }

        // Step 3: Instruct the Agent to look for this container and call sys_connect on it
        //         Note that the following code simply waits for a response from Agent on the status of running sys_connect 
        //         i.e. this is where we can use RPC 
        //         Once the container actually establishes a connection to the GCM, the container will send it's own init 
        //         message to the server which will call: handle_add_cgroup_to_ec

        std::cout << "[K8s LOG] Looking for node running container with image: " << app_image << std::endl;
        node_names.clear();
        ec::Facade::DeployFacade::Deploy::getNodesWithContainer(pod_name, node_names);
        node_ips.clear();
        ec::Facade::DeployFacade::Deploy::getNodeIPs(node_names, node_ips);

        for (const auto &node_ip : node_ips) {
            // Get the Agent with this node ip first (this needs to change to a singleton class)
            //for (const auto &agentClient : _ec->get_agent_clients()) {
            auto node_ip_om = om::net::ip4_addr::from_string(node_ip);
            const AgentClient* target_ac = acdb->get_agent_client_by_ip(node_ip_om);
            if (target_ac) {
                mtx.lock();
                auto itr = pod_conn_check.find(node_ip_om);
                if(itr == pod_conn_check.end()) {
                    pod_conn_check.insert(std::make_pair(node_ip_om, std::make_pair(1,0) )); //dep 1 cont
                } else {
                    itr->second.first++; //increase new pod deployment
                }
                mtx.unlock();

                std::string status;
                while(status != "Running") {
                    ec::Facade::DeployFacade::Deploy::getContainerStatus(pod_name, status);
                }
                std::cout << "POD STATUS1: " << status << std::endl;

                //Call sysconnect on pod
                msg_struct::ECMessage init_msg;
                init_msg.set_client_ip(gcm_ip); //IP of the GCM
                init_msg.set_req_type(4);
                init_msg.set_payload_string(pod_name + " "); // Todo: unknown bug where protobuf removes last character from this..
                init_msg.set_cgroup_id(0);

                res = ec::Facade::ProtoBufFacade::ProtoBuf::sendMessage(target_ac->get_socket(), init_msg);
                if(res < 0) {
                    std::cout << "[ERROR]: create_ec() - Error in writing to agent_clients socket. " << std::endl;
                    return __FAILED__;
                }
                std::cout << "got sendMessage back" << std::endl;

                /* TODO: Get back docker_id, cgroup_id - and note node IP
                 * keep track of all cgroups/pods that have successfully connected to GCM
                 * add docker_id to cgroup here.
                 * delete the get_Sc_from_agent call. we don't need it. delete for loop (but need add docker_id logic)
                 */
                msg_struct::ECMessage rx_msg;
                ec::Facade::ProtoBufFacade::ProtoBuf::recvMessage(target_ac->get_socket(), rx_msg);
                // This should check if the agent returns a docker id. Solved the bug where agent returns empty string and as a consequence, cadvsior returns stats from root container
                if (rx_msg.payload_string().empty()) {
                    std::cout << "[deployment error]: No docker id recieved from Agent. " << target_ac->get_agent_ip() << ". Check Agent Logs for more info. " << std::endl;
                    return __FAILED__;
                }
                if (rx_msg.rsrc_amnt() == (uint64_t) -1 ) {
                    std::cout << "[deployment error]: Error in creating a container on agent client with ip: " << target_ac->get_agent_ip() << ". Check Agent Logs for more info" << std::endl;
                    return __FAILED__;
                }
//                std::string docker_id = rx_msg.payload_string();
                //TODO: if I delete this print, GCM fails to read the payload string correctly about 50% of the time
                std::cout << "docker_id: " << rx_msg.payload_string() << std::endl;
                std::string docker_id = const_cast<std::string &>(rx_msg.payload_string());
                //TODO: wait until every pod has been connected to manager
//                std::cout << "rx docker_id from agent: " << docker_id << std::endl;
//                mtx.lock();

                std::unique_lock<std::mutex> lk(cv_mtx);
                cv.wait(lk, [this, node_ip_om] {
                    auto itr = pod_conn_check.find(node_ip_om);
                    std::cout << "create_ec() itr->first, second: " << itr->second.first << ", " << itr->second.second << std::endl;
                    return itr->second.first == itr->second.second;
                });

                _ec->get_sc_from_agent(target_ac, scs_per_agent);
                for (const auto &sc_id: scs_per_agent) {
                    if(std::count(scs_done.begin(), scs_done.end(), sc_id)) {	
                        continue;	
                    } else {	
                        std::cout << "current sc with id: " << sc_id << std::endl;	
                        _ec->get_subcontainer(sc_id).set_docker_id(docker_id);
                        std::cout << "docker id set: " << _ec->get_subcontainer(sc_id).get_docker_id() << std::endl;	
                        scs_done.push_back(sc_id);
                    }
                }
            } else {
                continue;
            }
        }

    }
    std::cout << "[EC LOG]: Done with create_ec()" << std::endl;
    
    return 0;
}

const ec::ElasticContainer& ec::ECAPI::get_elastic_container() const {
    if(_ec == nullptr) {
        std::cout << "[ERROR]: Must create _ec before accessing it" << std::endl;
        exit(EXIT_FAILURE);
    }
    return *_ec;
}

ec::ECAPI::~ECAPI() {
    delete _ec;
}

int ec::ECAPI::handle_add_cgroup_to_ec(const ec::msg_t *req, ec::msg_t *res, const uint32_t ip, int fd) {
    if(!req || !res) {
        std::cout << "ERROR. res or req == null in handle_add_cgroup_to_ec()" << std::endl;
        return __ALLOC_FAILED__;
    }
    // This is where we see the connection be initiated by a container on some node  
    auto *sc = _ec->create_new_sc(req->cgroup_id, ip, fd, req->rsrc_amnt, req->request); //update with throttle and quota
    if (!sc) {
        std::cerr << "[ERROR] Unable to create new sc object: Line 147" << std::endl;
        return __ALLOC_FAILED__;
    }

    int ret = _ec->insert_sc(*sc);
    _ec->incr_total_cpu(sc->sc_get_quota());
    _ec->update_fair_cpu_share();
    std::cout << "fair share: " << ec_get_fair_cpu_share() << std::endl;

    // And so once a subcontainer is created and added to the appropriate distributed container,
    // we can now create a map to link the container_id and agent_client
    
    std::cout << "[dbg]: Init. Added cgroup to _ec. cgroup id: " << *sc->get_c_id() << std::endl;
    AgentClientDB* acdb = AgentClientDB::get_agent_client_db_instance();
    auto agent_ip = sc->get_c_id()->server_ip;
    auto target_agent = acdb->get_agent_client_by_ip(agent_ip);
    std::cerr << "[dbg] Agent client ip: " << target_agent-> get_agent_ip() << std::endl;
    std::cerr << "[dbg] Agent ip: " << agent_ip << std::endl;
    if ( target_agent ){
//        mtx.lock();
        _ec->add_to_agent_map(*sc->get_c_id(), target_agent);
//        mtx.unlock();
    } else {
        std::cerr<< "[ERROR] SubContainer's node IP or Agent IP not found!" << std::endl;
    }

    std::lock_guard<std::mutex> lk(cv_mtx);
    auto itr = pod_conn_check.find(agent_ip);
    if(itr == pod_conn_check.end()) {
        std::cout << "[ECAPI ERROR]: Should not reach here. no agent ip found in pod_conn_check map" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    itr->second.second++;
    std::cout << "handle() itr->first, second: " << itr->second.first << ", " << itr->second.second << std::endl;
    cv.notify_one();

    res->request = 0; //giveback (or send back)
    return ret;
}

void ec::ECAPI::ec_decrement_memory_available(uint64_t mem_to_reduce) {
    _ec->ec_decrement_memory_available(mem_to_reduce);
}

uint64_t ec::ECAPI::get_memory_limit_in_bytes(const ec::SubContainer::ContainerId &container_id) {
    uint64_t ret = 0;
     // This is where we'll use cAdvisor instead of the agent comm to get the mem limit
//    std::cout << "CONTAINER ID USED: " << container_id << std::endl;
//    std::cerr << "[dbg] get_memory_limit_in_bytes: get the corresponding agent\n";
    AgentClient* ac = _ec->get_corres_agent(container_id);
    if(!ac) {
        std::cerr << "[ERROR] NO AgentClient found for container id: " << container_id << std::endl;
        return 0;
    }
    ec::SubContainer sc = _ec->get_subcontainer(container_id);

//    std::cout << "docker id used:" <<  sc.get_docker_id() << std::endl;
    ret = ec::Facade::MonitorFacade::CAdvisor::getContMemLimit(ac->get_agent_ip().to_string(), sc.get_docker_id());
    return ret;
}

uint64_t ec::ECAPI::get_memory_usage_in_bytes(const ec::SubContainer::ContainerId &container_id) {
    uint64_t ret = 0;
     // This is where we'll use cAdvisor instead of the agent comm to get the mem limit
//    std::cout << "CONTAINER ID USED: " << container_id << std::endl;
//    std::cerr << "[dbg] get_memory_limit_in_bytes: get the corresponding agent\n";
    AgentClient* ac = _ec->get_corres_agent(container_id);
    if(!ac) {
        std::cerr << "[ERROR] NO AgentClient found for container id: " << container_id << std::endl;
        return 0;
    }
    ec::SubContainer sc = _ec->get_subcontainer(container_id);

//    std::cout << "docker id used:" <<  sc.get_docker_id() << std::endl;
    ret = ec::Facade::MonitorFacade::CAdvisor::getContMemUsage(ac->get_agent_ip().to_string(), sc.get_docker_id());
    return ret;
}

uint64_t ec::ECAPI::get_machine_free_memory(const ec::SubContainer::ContainerId &container_id) {
    uint64_t ret = 0;
     // This is where we'll use cAdvisor instead of the agent comm to get the mem limit
    AgentClient* ac = _ec->get_corres_agent(container_id);  
    if(!ac) {
        std::cerr << "[ERROR] NO AgentClient found for container id: " << container_id << std::endl;
        return 0;
    }
    ec::SubContainer sc = _ec->get_subcontainer(container_id);
    ret = ec::Facade::MonitorFacade::CAdvisor::getMachineFreeMem(ac->get_agent_ip().to_string());
    return ret;
}

int64_t ec::ECAPI::get_cpu_quota_in_us(const ec::SubContainer::ContainerId &container_id) {
    uint64_t ret = 0;
    // This is where we'll use cAdvisor instead of the agent comm to get the mem limit
    AgentClient* ac = _ec->get_corres_agent(container_id);
    if(!ac) {
        std::cerr << "[ERROR] NO AgentClient found for container id: " << container_id << std::endl;
        return 0;
    }
    ec::SubContainer sc = _ec->get_subcontainer(container_id);

//    std::cout << "docker id used:" <<  sc.get_docker_id() << std::endl;
    ret = ec::Facade::MonitorFacade::CAdvisor::getContCPUQuota(ac->get_agent_ip().to_string(), sc.get_docker_id());
    return ret;
}


int64_t ec::ECAPI::set_sc_quota(ec::SubContainer *sc, uint64_t _quota, uint32_t seq_number) {
    if(!sc) {
        std::cout << "sc == NULL in manager set_sc_quota()" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    msg_struct::ECMessage msg_req;
    msg_req.set_req_type(0); //__CPU__
    msg_req.set_cgroup_id(sc->get_c_id()->cgroup_id);
    msg_req.set_request(seq_number);
    msg_req.set_quota(_quota);
    msg_req.set_payload_string("test");

//    std::cout << "updateing quota to (input, in msg_Req): (" << _quota << ", " << msg_req.quota() << ")" << std::endl;
    auto agent = _ec->get_corres_agent(*sc->get_c_id());
    if(!agent) {
        std::cerr << "agent for container == NULL" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    int64_t ret = agent->send_request(msg_req);
    return ret;

}

int64_t ec::ECAPI::get_sc_quota(ec::SubContainer *sc) {
    if(!sc) {
        std::cout << "sc == NULL in manager get_sc_quota()" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    uint32_t seq_number = 1;
    msg_struct::ECMessage msg_req;
    msg_req.set_req_type(7); //__READ_QUOTA__
    msg_req.set_cgroup_id(sc->get_c_id()->cgroup_id);
    msg_req.set_request(seq_number);
    msg_req.set_payload_string("test");

//    std::cout << "updateing quota to (input, in msg_Req): (" << _quota << ", " << msg_req.quota() << ")" << std::endl;
    auto agent = _ec->get_corres_agent(*sc->get_c_id());
    if(!agent) {
        std::cerr << "agent for container == NULL" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    int64_t ret = agent->send_request(msg_req);
    return ret;
}


int64_t ec::ECAPI::resize_memory_limit_in_bytes(ec::SubContainer::ContainerId container_id, uint64_t new_mem_limit) {
    uint64_t ret = 0;
    msg_struct::ECMessage msg_req;
    msg_req.set_req_type(6); //RESIZE_MEM_LIMIT
    msg_req.set_cgroup_id(container_id.cgroup_id);
    msg_req.set_payload_string("test");
    msg_req.set_rsrc_amnt(new_mem_limit);

    auto agent = _ec->get_corres_agent(container_id);

    if(!agent) {
        std::cerr << "[dbg] temp is NULL" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    
    ret = agent->send_request(msg_req);
    return ret;
}




