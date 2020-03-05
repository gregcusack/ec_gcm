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
        std::cout << "[Deploy Application Log] Created elastic container status: " << cont_create << " for app: " << app_name << " with image: " << app_images[i] <<std::endl;
    }
}

int ec::ECAPI::create_ec(std::string app_name, std::string app_image) {
    _ec = new ElasticContainer(manager_id, agent_clients);      

    /* This is the highest level of abstraction we want to provide to the end application developer. 
    
    Steps to create and deploy the "distributed container":
        1. Create a Pod deployment strategy
        2. Communicate with K8 REST API to deploy the pod on all nodes (based on default kube-scheduler)
        3. Send a request to the specific agent on a node to call sys_connect
    */

    // Step 1: Create a Pod on each of the nodes running an agent
    std::vector<AgentClient *> ec_agent_clients = _ec->get_agent_clients();
    int pod_creation;
    
//    std::string pod_name = app_name+"-"+app_image;
    std::string pod_name = app_name+"-pod-name";

    int ret;

    // Step 1: Generate a JSON file for each Pod definition
    std::cout << "[K8s LOG] Generating K8s Definition File " << std::endl;
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
    std::cerr << "[dbg] node ips are: " << node_ips[0] << std::endl;
    //for (const auto &agentClient : ec_agent_clients) {
    for (const auto &node_ip : node_ips) {
        // Get the Agent with this node ip first..
        for (const auto &agentClient : ec_agent_clients) {
            if (agentClient->get_agent_ip() == om::net::ip4_addr::from_string(node_ip)) {

                msg_struct::ECMessage init_msg;
//                init_msg.set_client_ip("192.168.6.10");
                init_msg.set_client_ip("10.0.2.15");
                init_msg.set_req_type(4);
                init_msg.set_payload_string(pod_name + " "); // Todo: unknown bug where protobuf removes last character from this..
                init_msg.set_cgroup_id(0);

                int tx_size = init_msg.ByteSizeLong()+4;
                char* tx_buf = new char[tx_size];
                google::protobuf::io::ArrayOutputStream arrayOut(tx_buf, tx_size);
                google::protobuf::io::CodedOutputStream codedOut(&arrayOut);
                codedOut.WriteVarint32(init_msg.ByteSizeLong());
                init_msg.SerializeToCodedStream(&codedOut);

                std::cout << "[EC Init] Sending Message to Agent at IP: " << agentClient->get_agent_ip()  << " with message of length: " << tx_size << std::endl; 
                if (write(agentClient->get_socket(), (void*) tx_buf, tx_size) < 0) {
                    std::cout << "[ERROR]: In Deploy_Container. Error in writing to agent_clients socket: " << std::endl;
                    return -1;
                }
                char rx_buffer[2048];
                bzero(rx_buffer, 2048);
                ret = read(agentClient->get_socket(), rx_buffer, 2048);
                if (ret <= 0) {
                    std::cout << "[ERROR]: GCM. Can't read from socket after deploying container" << std::endl;
                    return -2;
                }

                msg_struct::ECMessage rx_msg;
                google::protobuf::uint32 size;
                google::protobuf::io::ArrayInputStream ais(rx_buffer,4);
                CodedInputStream coded_input(&ais);
                coded_input.ReadVarint32(&size); 
                google::protobuf::io::ArrayInputStream arrayIn(rx_buffer, size);
                google::protobuf::io::CodedInputStream codedIn(&arrayIn);
                codedIn.ReadVarint32(&size);
                google::protobuf::io::CodedInputStream::Limit msgLimit = codedIn.PushLimit(size);
                rx_msg.ParseFromCodedStream(&codedIn);
                codedIn.PopLimit(msgLimit);
                
                std::cout << "[EC Init] Response from Agent: (Request_Type, Container Name, Container PID): (" << rx_msg.req_type() << ", " <<  rx_msg.payload_string() << ", " <<  rx_msg.rsrc_amnt() << ")" << std::endl;
                if ( rx_msg.rsrc_amnt() == (uint64_t) -1 ) {
                    std::cout << "[deployment error]: Error in creating a container on agent client with ip: " << agentClient->get_agent_ip() << ". Check Agent Logs for more info" << std::endl;
                    return -3;
                }
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

    std::cout << "[dbg]: IP from container -  " << ip << std::endl;
    // And so once a subcontainer is created and added to the appropriate distributed container,
    // we can now create a map to link the container_id and cgroup_id - this is the place to do that..
    std::cout << "[dbg]: Init. Added cgroup to _ec. cgroup id: " << *sc->get_c_id() << std::endl;
    auto agent_ip = sc->get_c_id()->server_ip;
    for (const auto &agentClient : _ec->get_agent_clients()) {
        std::cerr << "[dbg] Agent client ip: " << agentClient-> get_agent_ip() << std::endl;
        std::cerr << "[dbg] Agent ip: " << agent_ip << std::endl;
        if (agentClient->get_agent_ip() == agent_ip) {
            _ec->add_to_agent_map(*sc->get_c_id(), agentClient);
        }
    }
    std::cerr << "[dbg] Agent client map in the elastic container ? " << _ec->get_agent_clients()[0]->get_agent_ip() << " " << _ec->get_agent_clients()[0]->get_socket() << std::endl;

    res->request = 0; //giveback (or send back)
    return ret;

}

void ec::ECAPI::ec_decrement_memory_available(uint64_t mem_to_reduce) {
    _ec->ec_decrement_memory_available(mem_to_reduce);
}

uint64_t ec::ECAPI::get_memory_limit_in_bytes(ec::SubContainer::ContainerId container_id) {
    
    uint64_t ret = 0;

    //initialize request body
    msg_struct::ECMessage msg_req;
    msg_req.set_req_type(5); //MEM_LIMIT_IN_BYTES 
    msg_req.set_cgroup_id(container_id.cgroup_id);
    msg_req.set_payload_string("test");

    std::cerr << "[dbg] get_memory_limit_in_bytes: get the corresponding agent\n";
    std::cerr << "[dbg] Getting the agent clients: " << _ec->get_agent_clients()[0]->get_socket() << std::endl;
    auto temp = _ec->get_corres_agent(container_id);
    if(!temp) {
        std::cerr << "[dbg] temp is NULL" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    
    std::cerr << "[dbg] temp sockfd is : " << temp->get_socket() << std::endl;
    ret = temp->send_request(msg_req);
    // delete(_req);
    return ret;

}




