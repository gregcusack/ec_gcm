//
// Created by greg on 9/11/19.
//

#include "ElasticContainer.h"


ec::ElasticContainer::ElasticContainer(uint32_t _ec_id) : ec_id(_ec_id) {}

ec::ElasticContainer::ElasticContainer(uint32_t _ec_id, std::vector<AgentClient *> &_agent_clients)
    : ec_id(_ec_id), agent_clients(_agent_clients) {

    //TODO: change num_agents to however many servers we have. IDK how to set it rn.

    _mem = global::stats::mem();
    _cpu = global::stats::cpu();

    std::cout << "[Elastic Container Log] runtime_remaining on init: " << _cpu.get_runtime_remaining() << std::endl;
    std::cout << "[Elastic Container Log] memory_available on init: " << _mem.get_mem_available() << std::endl;

    test_file.open("test_file.txt", std::ios_base::app);

    //test
    flag = 0;
    subcontainers = subcontainer_map();
    sc_agent_map = subcontainer_agent_map();
}

ec::SubContainer *ec::ElasticContainer::create_new_sc(const uint32_t cgroup_id, const uint32_t host_ip, const int sockfd) {
    return new SubContainer(cgroup_id, host_ip, sockfd);
}

const ec::SubContainer &ec::ElasticContainer::get_subcontainer(ec::SubContainer::ContainerId &container_id) {
    auto itr = subcontainers.find(container_id);
    if(itr == subcontainers.end()) {
        std::cout << "ERROR: No EC with manager_id: " << ec_id << ". Exiting...." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return *itr->second;
}

int ec::ElasticContainer::insert_sc(ec::SubContainer &_sc) {
    if (subcontainers.find(*_sc.get_c_id()) != subcontainers.end()) {
        std::cout << "This SubContainer already exists! Can't allocate identical one!" << std::endl;
        //TODO: should delete sc
        return __ALLOC_FAILED__;
    }
    subcontainers.insert({*(_sc.get_c_id()), &_sc});

    return __ALLOC_SUCCESS__;
}

// int ec::ElasticContainer::int insert_pid(int pid){
//     if (pids == NULL) {
//         return __ALLOC_FAILED__;
//     }
//     pids.push_back(pid);
//     return __ALLOC_SUCCESS__;
// }

// int ec::ElasticContainer::int get_pids(){
//     if (pids == NULL) {
//         return __ALLOC_FAILED__;
//     }
//     return pids;
// }

uint64_t ec::ElasticContainer::refill_runtime() {
    return _cpu.refill_runtime();
}

ec::ElasticContainer::~ElasticContainer() {
    for(auto &i : subcontainers) {
        delete i.second;
    }
    subcontainers.clear();
}

//K8s Helper Functions

int ec::ElasticContainer::deploy_pod(const json::value pod_json) {
    std::cout << "[K8s Log] Sending k8s a request to create Pod.."  << std::endl;

    json::value json_return;
    // Assumes that there's a k8s proxy running on localhost, port 8000
    web::http::client::http_client client("http://localhost:8000/api/v1/namespaces/default/pods");
    client.request(web::http::methods::POST, U("/"), pod_json)
    .then([](const web::http::http_response& response) {
        return response.extract_json(); 
    })
    .then([&json_return](const pplx::task<web::json::value>& task) {
        try {
            json_return = task.get();
        }
        catch (const web::http::http_exception& e) {                    
            std::cout << "error " << e.what() << std::endl;
        }
    })
    .wait();

    std::cout << "[K8s Log] Received K8s API response" << std::endl;
    //std::cout << json_return.serialize() << std::endl;

    // Prerit Todo: Is there anyway to tell from k8s right away if the Pod Failed at deployment? Look into this..
    // If so, return -1 for a failed pod deployment
    return 0;
}

std::vector<std::string> ec::ElasticContainer::get_nodes_with_pod(const std::string pod_name) {
    std::vector<std::string> result;
    std::cout << "Sending k8s a request to get the node with the pod name: "  << pod_name <<  std::endl;

    json::value json_return;
    // Assumes that there's a k8s proxy running on localhost, port 8000
    web::http::client::http_client client("http://localhost:8000/api/v1/namespaces/default/pods/" + pod_name);

    //std::cout << "[K8s RESPONSE]" << std::endl;
    client.request(web::http::methods::GET, U("/"))
    .then([](const web::http::http_response& response) {
        return response.extract_json(); 
    })
    .then([&json_return](const pplx::task<web::json::value>& task) {
        try {
            json_return = task.get();
        }
        catch (const web::http::http_exception& e) {                    
            std::cout << "error " << e.what() << std::endl;
        }
    })
    .wait();

    // THIS NEEDS TO BE REVISED TO SEE WHAT HAPPENS IF TWO NODES ARE RUNNING THE SAME POD NAME..
    //utility::string_t jsonval = json_return.serialize();
    //auto array = json_return.at(U("spec")).as_string();
    auto array = json_return.at(U("spec")).at(U("nodeName")).as_string();
    // for (int i = 0; i < array.size(); i++) {
    //     result.push_back(array[i].as_string());
    //     //std::cout << array[i].as_string << std::endl;
    // }
    //std::cout <<array<< std::endl;

    result.push_back(array);
    return result;
}

std::vector<std::string> ec::ElasticContainer::get_nodes_ips(const std::vector<std::string> node_names) {
    std::vector<std::string> result;
    std::cout << "Sending k8s a request to get the node ips.. " <<  std::endl;

    json::value json_return;
    // Assumes that there's a k8s proxy running on localhost, port 8000
    for (auto node : node_names) {
        web::http::client::http_client client("http://localhost:8000/api/v1/nodes/" + node);

        //std::cout << "[K8s RESPONSE]" << std::endl;
        client.request(web::http::methods::GET, U("/"))
        .then([](const web::http::http_response& response) {
            return response.extract_json(); 
        })
        .then([&json_return](const pplx::task<web::json::value>& task) {
            try {
                json_return = task.get();
            }
            catch (const web::http::http_exception& e) {                    
                std::cout << "error " << e.what() << std::endl;
            }
        })
        .wait();

        //utility::string_t jsonval = json_return.serialize();
        auto jsonval = json_return.at(U("status")).at(U("addresses")).as_array();
        for (int i = 0; i < jsonval.size(); i++) {
            if (jsonval[i].at(U("type")).as_string() == "InternalIP") {
                //std::cout << jsonval[i].at(U("address")).as_string() << std::endl;
                result.push_back(jsonval[i].at(U("address")).as_string());
            }
        }
    }

    return result;
}