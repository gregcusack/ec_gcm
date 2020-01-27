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

    std::cout << "runtime_remaining on init: " << _cpu.get_runtime_remaining() << std::endl;
    std::cout << "memory_available on init: " << _mem.get_mem_available() << std::endl;

    test_file.open("test_file.txt", std::ios_base::app);

    //test
    flag = 0;

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

json::value ec::ElasticContainer::generate_pod_json(const std::string pod_name) {
    // Create a JSON object (the pod)
    json::value pod;
    pod[U("kind")] = json::value::string(U("Pod"));
    pod[U("apiVersion")] = json::value::string(U("v1"));

    // Create a JSON object (the metadata)
    json::value metadata;
    metadata[U("namespace")] = json::value::string(U("default"));
    metadata[U("name")] = json::value::string(U(pod_name));

    // Create a JSON object (the metadata label)
    json::value metadata_label;
    metadata_label[U("name")] = json::value::string(U(pod_name));

    metadata[U("labels")] = metadata_label;

    pod[U("metadata")] = metadata;

    // Now we worry about the specs..
    json::value cont1;
    cont1[U("name")] = json::value::string(U(pod_name));
    // Default image is nginx
    cont1[U("image")] = json::value::string(U("nginx"));

    json::value cont1_port;
    cont1_port[U("containerPort")] = json::value::number(U(80));

    json::value ports;
    ports[0] = cont1_port;
    cont1[U("ports")] = ports;

    // Create the items array
    json::value containers;
    containers[0] = cont1;

    json::value cont;
    cont[U("containers")] = containers;

    pod[U("spec")] = cont;

    // Write the current JSON value to a stream with the native platform character width
    utility::stringstream_t stream;
    pod.serialize(stream);

    return pod;
}

int ec::ElasticContainer::deploy_pod(const json::value pod_json) {
    std::cout << "Sending k8s a request to create Pod.."  << std::endl;

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

    std::cout << "JSON Response: " << std::endl;
    std::cout << json_return.serialize() << std::endl;

    // Prerit Todo: Is there anyway to tell from k8s right away if the Pod Failed at deployment? Look into this..
    // If so, return -1 for a failed pod deployment
    return 0;
}
