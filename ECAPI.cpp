//
// Created by greg on 9/12/19.
//

#include "ECAPI.h"

//ec::ECAPI::ECAPI(uint32_t _ec_id)
//    : manager_id(_ec_id) {
 //   // _ec = new ElasticContainer(manager_id, agent_clients);
//}

int ec::ECAPI::create_ec(const std::string &app_name, const std::vector<std::string> &app_images, const std::string &gcm_ip) {
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
    std::string pod_name;

    ec::Facade::JSONFacade::json jsonFacade;
    ec::Facade::DeployFacade::Deploy deployment;
    std::vector<SubContainer::ContainerId> scs_per_agent;
    std::vector<SubContainer::ContainerId> scs_done;
    ec::Facade::ProtoBufFacade::ProtoBuf protoFacade;
    
    for (const auto &app_image: app_images) {
        // Step 1: Create a Pod on each of the nodes running an agent - k8s takes care of this
        //         todo: this will change we implement k8-yaml support (still brainstorming how best to do that)           
        std::cout << "[DEPLOY LOG] Generating JSON POD File for image: " << app_image << std::endl;
        jsonFacade.createJSONPodDef(app_name, app_image, response);
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
        pod_name = app_name + "-" + app_image;
        node_names.clear();
        deployment.getNodesWithContainer(pod_name, node_names);
        node_ips.clear();
        deployment.getNodeIPs(node_names, node_ips);


        for (const auto node_ip : node_ips) {
            // Get the Agent with this node ip first (this needs to changed to a singleton class)
            for (const auto &agentClient : _ec->get_agent_clients()) { 
                if (agentClient->get_agent_ip() == om::net::ip4_addr::from_string(node_ip)) {
                    scs_per_agent.clear();
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
                    while (rx_msg.payload_string().size() == 0) {
                        protoFacade.recvMessage(agentClient->get_socket(), rx_msg);
                    }
                    // Wait here until subcontainer has been added to the agent client map
                    mtx.lock();
                    _ec->get_sc_from_agent(agentClient, scs_per_agent);
                    for (auto &sc_id: scs_per_agent) {
                        if(std::count(scs_done.begin(), scs_done.end(), sc_id)) {
                            continue;
                        } else {
                            std::cout << "current sc with id: " << sc_id << std::endl;
                            _ec->get_subcontainer(sc_id).set_docker_id(rx_msg.payload_string());
                            std::cout << "docker id set: " << _ec->get_subcontainer(sc_id).get_docker_id() << std::endl;
                            scs_done.push_back(sc_id);
                        }
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


int ec::ECAPI::handle_add_cgroup_to_ec(ec::msg_t *res, uint32_t cgroup_id, const uint32_t ip, int fd) {
    if(!res) {
        std::cout << "ERROR. res == null in handle_add_cgroup_to_ec()" << std::endl;
        return __ALLOC_FAILED__;
    }
    // This is where we see the connection be initiated by a container on some node  
    auto *sc = _ec->create_new_sc(cgroup_id, ip, fd);
    
    if (sc == NULL) {
        std::cerr << "[ERROR] Unable to create new sc object: Line 147" << std::endl;
        return __ALLOC_FAILED__;
    }
    int ret = _ec->insert_sc(*sc);
    std::cout << "[dbg]: IP from container -  " << ip << std::endl;


    // And so once a subcontainer is created and added to the appropriate distributed container,
    // we can now create a map to link the container_id and agent_client
    
    std::cout << "[dbg]: Init. Added cgroup to _ec with id: " << _ec->get_ec_id() << ". cgroup id: " << *sc->get_c_id() << std::endl;
    std::cout << "[dbg]: Map Size at Insert " << _ec->get_subcontainers().size() << std::endl;

    for (const auto &agentClient : _ec->get_agent_clients()) {
        if (agentClient->get_agent_ip() == sc->get_c_id()->server_ip) {
            _ec->add_to_agent_map(*sc->get_c_id(), agentClient);
            mtx.unlock();
            std::cout << "[EVENT LOG]: Added subcontainer to Agent Map" << std::endl;
        } else {
            continue;
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

    // This is where we'll use cAdvisor instead of the agent comm to get the mem limit
    std::cout << "CONTAINER ID USED: " << container_id << std::endl;
    std::cerr << "[dbg] get_memory_limit_in_bytes: get the corresponding agent\n";
    AgentClient* ac = _ec->get_corres_agent(container_id);
    if(ac == NULL)
         std::cerr << "[ERROR] NO AgentClient found for container id: " << container_id << std::endl;
    ec::SubContainer sc = _ec->get_subcontainer(container_id);

    ec::Facade::MonitorFacade::CAdvisor monitor_obj;
    std::cout << "docker id used:" <<  sc.get_docker_id() << std::endl;
    ret = monitor_obj.getContMemLimit(ac->get_agent_ip().to_string(), sc.get_docker_id());
    return ret;
}





