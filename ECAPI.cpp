//
// Created by greg on 9/12/19.
//

#include "ECAPI.h"

ec::ECAPI::ECAPI(uint32_t _ec_id, std::vector<AgentClient *> &_agent_clients)
    : manager_id(_ec_id), agent_clients(_agent_clients) {
    // _ec = new ElasticContainer(manager_id, agent_clients);
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

    char rx_buffer[2048] = {0};
    int ret;

    for (const auto &agentClient : ec_agent_clients) {
        // Step 1: Generate a JSON file for each Pod definition
        json::value json_output = _ec->generate_pod_json(pod_name, app_image);
        // std::cout << "JSON File output: " << json_output << std::endl;
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
        
        msg_struct::ECMessage init_msg;
        init_msg.set_client_ip("192.168.6.10");
        init_msg.set_req_type(4);
        init_msg.set_payload_string(pod_name + " "); // Todo: unknown bug where protobuf removes last character from this..
        init_msg.set_cgroup_id(0);

        int tx_size = init_msg.ByteSizeLong()+4;
        char* tx_buf = new char[tx_size];
        google::protobuf::io::ArrayOutputStream arrayOut(tx_buf, tx_size);
        google::protobuf::io::CodedOutputStream codedOut(&arrayOut);
        codedOut.WriteVarint32(init_msg.ByteSizeLong());
        init_msg.SerializeToCodedStream(&codedOut);

        std::cout << "Message length is: " << tx_size << std::endl; 
        if (write(agentClient->get_socket(), (void*) tx_buf, tx_size) < 0) {
            std::cout << "[ERROR]: In Deploy_Container. Error in writing to agent_clients socket: " << std::endl;
            return -1;
        }

        ret = read(agentClient->get_socket(), rx_buffer, sizeof(rx_buffer));
        if (ret <= 0) {
            std::cout << "[ERROR]: GCM. Can't read from socket after deploying container" << std::endl;
            return -2;
        }
        
        msg_struct::ECMessage rx_msg;

        google::protobuf::uint32 size;
        google::protobuf::io::ArrayInputStream ais(rx_buffer,4);
        CodedInputStream coded_input(&ais);
        coded_input.ReadVarint32(&size); //Decode the HDR and get the size
        google::protobuf::io::ArrayInputStream arrayIn(rx_buffer, size);
        google::protobuf::io::CodedInputStream codedIn(&arrayIn);
        codedIn.ReadVarint32(&size);
        google::protobuf::io::CodedInputStream::Limit msgLimit = codedIn.PushLimit(size);
        rx_msg.ParseFromCodedStream(&codedIn);
        codedIn.PopLimit(msgLimit);

        std::cout << "response from agent: request type, container name, Container_PID (unsigned -1 means error): " << rx_msg.req_type() << ", " <<  rx_msg.payload_string() << ", " <<  rx_msg.rsrc_amnt()  << std::endl;
        
        if ( rx_msg.rsrc_amnt() == (uint64_t) -1 ) {
            std::cout << "[deployment error]: Error in creating a container on agent client with ip: " << agentClient->get_agent_ip() << ". Check Agent Logs for more info" << std::endl;
            return -3;
        }

        // Add PID of the container to the _ec map..
        // _ec->insert_pid(rx_msg.rsrc_amnt());
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

//int ec::ECAPI::handle_req(const char *buff_in, char *buff_out, uint32_t host_ip, int clifd) {
int ec::ECAPI::handle_req(const msg_t *req, msg_t *res, uint32_t host_ip, int clifd) {
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
            ret = handle_cpu_req(req, res);
            break;
        case _INIT_:
            ret = handle_add_cgroup_to_ec(res, req->cgroup_id, host_ip, clifd);
            break;
        default:
            std::cout << "[Error]: ECAPI: " << manager_id << ". Handling memory/cpu request failed!" << std::endl;
    }
    return ret;
}

int ec::ECAPI::handle_add_cgroup_to_ec(ec::msg_t *res, uint32_t cgroup_id, const uint32_t ip, int fd) {
    if(!res) {
        std::cout << "ERROR. res == null in handle_add_cgroup_to_ec()" << std::endl;
        return __ALLOC_FAILED__;
    }
    // This is where we see the connection be initiated by a container on some node  
    auto *sc = _ec->create_new_sc(cgroup_id, ip, fd);
    int ret = _ec->insert_sc(*sc);
    // And so once a subcontainer is created and added to the appropriate distributed container,
    // we can now create a map to link the container_id and cgroup_id - this is the place to do that..
    
    std::cout << "[dbg]: Init. Added cgroup to _ec. cgroup id: " << *sc->get_c_id() << std::endl;
    res->request = 0; //giveback (or send back)
    return ret;
}

void ec::ECAPI::ec_decrement_memory_available(uint64_t mem_to_reduce) {
    _ec->ec_decrement_memory_available(mem_to_reduce);
}





