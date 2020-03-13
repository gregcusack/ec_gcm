//
// Created by greg on 9/12/19.
//

#include "ECAPI.h"

//ec::ECAPI::ECAPI(uint32_t _ec_id)
//    : manager_id(_ec_id) {
 //   // _ec = new ElasticContainer(manager_id, agent_clients);
//}

int ec::ECAPI::create_ec(const std::string &app_name, const std::vector<std::string> &app_images, const std::vector<std::string> &pod_names, const std::string &gcm_ip) {
    _ec = new ElasticContainer(manager_id, agent_clients);      

    /* This is the highest level of abstraction provided to the end application developer.
    Steps to create and deploy the "distributed container":
        1. Create a Pod deployment strategy
        2. Communicate with K8 REST API to deploy the pod on all nodes (based on default kube-scheduler)
        3. Send a request to the specific agent on a node to call sys_connect
    */
    int pod_creation;

    int res;
    std::string response;
    std::vector<std::string> node_names;
    std::vector<std::string> node_ips;

    ec::Facade::JSONFacade::json jsonFacade;
    ec::Facade::DeployFacade::Deploy deployment;
    uint32_t podNameIndex = 0;
    for (const auto &app_image: app_images) {
        // Step 1: Create a Pod on each of the nodes running an agent - k8s takes care of this
        //         todo: this will change we implement k8-yaml support (still brainstorming how best to do that)           
        std::cout << "[DEPLOY LOG] Generating JSON POD File for image: " << app_image << std::endl;
        //TODO: need to add pod_name config so it only contains alphanumeric characters
        std::string pod_name = pod_names[podNameIndex];
        jsonFacade.createJSONPodDef(app_name, app_image, pod_name, response);
        podNameIndex++;
        // Step 2
        std::cout << "[DEPLOY LOG] Deploying Container With Image: " << app_image << std::endl;
        pod_creation = deployment.deployContainers(response);
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
//        pod_name = app_name + "-" + app_image;
        node_names.clear();
        deployment.getNodesWithContainer(pod_name, node_names);
        node_ips.clear();
        deployment.getNodeIPs(node_names, node_ips);

        for (const auto &node_ip : node_ips) {
            // Get the Agent with this node ip first (this needs to change to a singleton class)
            for (const auto &agentClient : _ec->get_agent_clients()) { 
                if (agentClient->get_agent_ip() == om::net::ip4_addr::from_string(node_ip)) {

                    ec::Facade::ProtoBufFacade::ProtoBuf protoFacade;
                    
                    msg_struct::ECMessage init_msg;
                    init_msg.set_client_ip(gcm_ip); //IP of the GCM
                    init_msg.set_req_type(4);
                    init_msg.set_payload_string(pod_name + " "); // Todo: unknown bug where protobuf removes last character from this..
                    init_msg.set_cgroup_id(0);

                    res = protoFacade.sendMessage(agentClient->get_socket(), init_msg);
                    if(res < 0) {
                        std::cout << "[ERROR]: create_ec() - Error in writing to agent_clients socket. " << std::endl;
                        return __FAILED__;
                    }

                    msg_struct::ECMessage rx_msg;
                    protoFacade.recvMessage(agentClient->get_socket(), rx_msg);

                    if (rx_msg.rsrc_amnt() == (uint64_t) -1 ) {
                        std::cout << "[deployment error]: Error in creating a container on agent client with ip: " << agentClient->get_agent_ip() << ". Check Agent Logs for more info" << std::endl;
                        return __FAILED__;
                    }
                } else {
                    continue;
                }
            }        
        }

    }
    
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
    // we can now create a map to link the container_id and cgroup_id - this is the place to do that..
    std::cout << "[dbg]: Init. Added cgroup to _ec with id: " << _ec->get_ec_id() << ". cgroup id: " << *sc->get_c_id() << std::endl;
    std::cout << "[dbg]: Map Size at Insert " << _ec->get_subcontainers().size() << std::endl;

    for (const auto &agentClient : _ec->get_agent_clients()) {
        //std::cerr << "[dbg] Agent client ip: " << agentClient-> get_agent_ip() << std::endl;
        //std::cerr << "[dbg] Agent ip: " <<  sc->get_c_id()->server_ip << std::endl;
        if (agentClient->get_agent_ip() == sc->get_c_id()->server_ip) {
            _ec->add_to_agent_map(*sc->get_c_id(), agentClient);
            std::cout << "[EVENT LOG]: Added subcontainer to Agent Map" << std::endl;
        }
    }
    res->request = 0; //giveback (or send back)
    return ret;

}

void ec::ECAPI::ec_decrement_memory_available(uint64_t mem_to_reduce) {
    _ec->ec_decrement_memory_available(mem_to_reduce);
}

uint64_t ec::ECAPI::get_memory_limit_in_bytes(const ec::SubContainer::ContainerId &container_id) {
    
    uint64_t ret = 0;
    //initialize request body
    msg_struct::ECMessage msg_req;
    msg_req.set_req_type(5); //MEM_LIMIT_IN_BYTES 
    msg_req.set_cgroup_id(container_id.cgroup_id);
    msg_req.set_payload_string("test");
    std::cerr << "[dbg] get_memory_limit_in_bytes: get the corresponding agent\n";
    std::cerr << "[dbg] Getting the agent clients: " << _ec->get_agent_clients()[0]->get_socket() << std::endl;
    auto agent = _ec->get_corres_agent(container_id);
    if(!agent) {
        std::cerr << "[dbg] agent is NULL" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    
    ret = agent->send_request(msg_req);
    return ret;
}


int64_t ec::ECAPI::set_sc_quota(ec::SubContainer *sc, uint64_t _quota) {
    if(!sc) {
        std::cout << "sc == NULL in manager set_sc_quota()" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    msg_struct::ECMessage msg_req;
    msg_req.set_req_type(0); //__CPU__
    msg_req.set_cgroup_id(sc->get_c_id()->cgroup_id);
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

bool ec::ECAPI::resize_memory_limit_in_bytes(ec::SubContainer::ContainerId container_id, uint64_t new_mem_limit) {
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
    return ret == 0;
}




