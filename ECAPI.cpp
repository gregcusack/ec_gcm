//
// Created by greg on 9/12/19.
//

#include "ECAPI.h"

//ec::ECAPI::ECAPI(uint32_t _ec_id)
//    : manager_id(_ec_id) {
 //   // _ec = new ElasticContainer(manager_id, agent_clients);
//}

void ec::ECAPI::deploy_application(std::string app_name, std::vector<std::string> app_images){
    // And this is where we can actually now "deploy" the distributed containers
    // instead of the current model of waiting for them to be created on the hosts
    /*if (manager == NULL) {
        std::cout << "error in finding manager reference.."  << std::endl;
    }*/
    for (int i=0; i<app_images.size(); i++) {
        int cont_create = create_ec(app_name, app_images[i]);
        std::cout << "Created elastic container status: " << cont_create << " for app: " << app_name << " with image: " << app_images[i] <<std::endl;
    }
}

int ec::ECAPI::create_ec(std::string app_name, std::string app_image) {
    _ec = new ElasticContainer(manager_id, agent_clients);       //pass in gcm_ip for now

    /* This is the highest level of abstraction we want to provide to the end application developer. 
       i.e. Some information we might need from the application developer:
            - Pod Name
            - Container application image
            - Application Limits (CPU, Memory, IO)
            - Container Ports
            - etc.
    
    Steps to create and deploy the "distributed container":
        1. Create a Pod JSON on each of the nodes associated with the agent_clients using k8s
            - Outstanding questions: are the agent_clients specific to this EC object or the entire GCM?
            - How should the pods eventually be distributed on the containers? i.e. Do we need to modify the k8s scheduler?
                - 1st step: use the default kube-scheduler
        2. Communicate with K8 REST API to deploy the pod on that agent
        3. Send a request to specific to call sys_connect on that created container
    */

    // Step 1: Create a Pod on each of the nodes running an agent
    std::vector<AgentClient *> ec_agent_clients = _ec->get_agent_clients();
    int pod_creation;
    
    // PreritO Todo: Expand support for msg_struct to send strings and not just uints
    std::string pod_name = app_name+"-"+app_image; 
    uint64_t pod_name_uint64;
    char* end;
    pod_name_uint64 = strtoull( pod_name.c_str(), &end,10 );

    char rx_buffer[2048] = {0};
    int ret;
    msg_t* rx_resp;

    // std::cout << "Agent IP" << agentClient->get_agent_ip() << std::endl;

    // Step 1: Generate a JSON file for each Pod definition
    std::cout << "Generating k8s JSON " << std::endl;
    json::value json_output = _ec->generate_pod_json(pod_name, app_image);
    //std::cout << "JSON File output: " << json_output << std::endl;
    // Step 2: Communicate with K8 REST API to deploy the pod on that agent
    //         This is where we might have to deal with some way to correspond the agent ip and k8 node name..
    //         so we can deploy to that specific node. (testing with one node in k8s cluster doesn't face this issue)
    
    pod_creation = _ec->deploy_pod(json_output);
    if (pod_creation != 0) {
        std::cout << "[k8s deploy error]:  Error in creating a Pod via k8s.. Exiting" << std::endl;
        return -1;
    }
    
    
    // Step 3: Instruct the Agent to look for this container and call sys_connect on it
    //         Note that the following code simply waits for a response from Agent on the status of running sys_connect 
    //         i.e. this is where we can use RPC 
    //         Once the container actually establishes a connection to the GCM, the container will send it's own init 
    //         message to the server which will call: handle_add_cgroup_to_ec
    
    // Get the Node name here from k8s API so that we get the IP and communicate with that appropriately:
    
    std::vector<std::string> node_names = _ec->get_nodes_with_pod(pod_name);
    std::vector<std::string> node_ips = _ec->get_nodes_ips(node_names);
    
    //for (const auto &agentClient : ec_agent_clients) {
    for (const auto node_ip : node_ips) {
        // Get the Agent with this node ip first..
        for (const auto &agentClient : ec_agent_clients) {
            if (agentClient->get_agent_ip() == om::net::ip4_addr::from_string(node_ip)) {
                
                int pod_name_uint64t;
                std::istringstream iss (pod_name);
                iss >> pod_name_uint64t;

                msg_t *init_cont_msg = new msg_t;
                //init_cont_msg->client_ip = om::net::ip4_addr::reverse_byte_order(om::net::ip4_addr::from_string("192.168.6.10"));
                init_cont_msg->client_ip = om::net::ip4_addr::from_string("128.105.144.138");
                init_cont_msg->cgroup_id = 0;
                init_cont_msg->req_type = 4;
                init_cont_msg->rsrc_amnt = 0;
                init_cont_msg->request = 1;
                init_cont_msg->cont_name = pod_name_uint64t;

                if (app_image == "nginx"){
                    init_cont_msg->runtime_remaining = 1;
                } else if (app_image == "redis") {
                    init_cont_msg->runtime_remaining = 2;
                } else {
                    init_cont_msg->runtime_remaining = 1;
                }

                if (write(agentClient->get_socket(), (char *) init_cont_msg, sizeof(*init_cont_msg)) < 0) {
                    std::cout << "[ERROR]: In Deploy_Container. Error in writing to agent_clients socket: " << std::endl;
                    return -1;
                }

                ret = read(agentClient->get_socket(), rx_buffer, sizeof(rx_buffer));
                if (ret <= 0) {
                    std::cout << "[ERROR]: GCM. Can't read from socket after deploying container" << std::endl;
                    return -2;
                }
                rx_resp = (msg_t *) rx_buffer;
                std::cout << "response from agent (client ip, request type, cont_id): " << rx_resp->client_ip << "," << rx_resp->req_type << ", " <<  rx_resp->rsrc_amnt << std::endl;
                // if (rx_resp->req_type == init_cont_msg->req_type && rx_resp->rsrc_amnt == 1) {
                //     std::cout << "[deploy error]: Error in creating a container on agent client with ip: " << agentClient->get_agent_ip() << ". Check Agent Logs for more info" << std::endl;
                //     return -3;
                // }
                
            } else {
                continue;
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
    std::cout << "[dbg]: HERE" << std::endl;
    
    if (sc == NULL) {
        std::cout << "ERROR. res == null in handle_add_cgroup_to_ec()" << std::endl;
        return __ALLOC_FAILED__;
    }
    int ret = _ec->insert_sc(*sc);
    std::cout << "[dbg]: IP from container -  " << ip << std::endl;

    // And so once a subcontainer is created and added to the appropriate distributed container,
    // we can now create a map to link the container_id and cgroup_id - this is the place to do that..
    
    std::cout << "[dbg]: Init. Added cgroup to _ec. cgroup id: " << *sc->get_c_id() << std::endl;
    std::vector<AgentClient *> ec_agent_clients = _ec->get_agent_clients();
    auto agent_ip = sc->get_c_id()->server_ip;
    for (const auto &agentClient : ec_agent_clients) {
        if (agentClient->get_agent_ip() == agent_ip) {
            _ec->add_to_agent_map(*sc->get_c_id(), agentClient);
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

uint64_t ec::ECAPI::get_memory_limit_in_bytes(ec::SubContainer::ContainerId container_id) {
    struct msg_t* _req = new struct msg_t;
    uint64_t ret = 0;
    //initialize request body
    _req->cgroup_id = container_id.cgroup_id;
    // //TODO: change to enumeration
    _req->req_type = 5; //MEM_LIMIT_IN_BYTES
    std::cerr << "[dbg] get_memory_limit_in_bytes: get the corresponding agent\n";
    ret = _ec->get_corres_agent(container_id)->send_request(_req)[0];
    // delete(_req);
    return ret;

}




