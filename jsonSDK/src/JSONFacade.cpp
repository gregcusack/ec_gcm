#include "../include/JSONFacade.h"

std::unordered_map<std::string, uint64_t> ec::Facade::JSONFacade::json::_specs;

void ec::Facade::JSONFacade::json::parseAppName() {
    if(_val.is_null()) {
        return;
    }
   _app_name = _val.at(U("name")).as_string();
}

void ec::Facade::JSONFacade::json::parseAppImages() {
    if(_val.is_null()) {
        return;
    }
    auto app_images = _val.at(U("images")).as_array();
    for(const auto &i : app_images) {
        _app_images.push_back(i.as_string());
    }
}

void ec::Facade::JSONFacade::json::parseIPAddresses() {
    if(_val.is_null()) {
        return;
    }
    auto agent_ips = _val.at(U("agentIPs")).as_array();
    for(const auto &i : agent_ips) {
        _agent_ips.push_back(i.as_string());
    }
}

void ec::Facade::JSONFacade::json::parseNumTenants() {
    if(_val.is_null()) {
        return;
    }
    _num_tenants = _val.at(U("tenants")).as_integer();
}

void ec::Facade::JSONFacade::json::parseNumContainers() {
    if(_val.is_null()) {
        return;
    }
    _num_containers = _val.at(U("num_containers")).as_integer();
}


void ec::Facade::JSONFacade::json::parsePodNames() {
    if(_val.is_null()){
        return;
    }
    auto pod_names = _val.at(U("pod-names")).as_array();
    for(const auto &i : pod_names) {
        _pod_names.push_back(i.as_string());
    }
}

void ec::Facade::JSONFacade::json::parseGCMIPAddress() {
    if(_val.is_null()) {
        return;
    }
   _gcm_ip = _val.at(U("gcmIP")).as_string();
}

void ec::Facade::JSONFacade::json::parseSpecs() {
    for(const auto &i : _val.at(U("specs")).as_object()){
        _specs.insert({i.first, i.second.as_integer()});
    }
}

int ec::Facade::JSONFacade::json::parseFile(const std::string &fileName) {
    try {
        utility::ifstream_t      file_stream(fileName);                                
        utility::stringstream_t  stream;                                         
        if (file_stream) {                                                    
            stream << file_stream.rdbuf();                                        
            file_stream.close();                                            
        }
        _val = web::json::value::parse(stream);
        parseAppName();
        parseIPAddresses();
        parseGCMIPAddress();
        parseNumTenants();
        parseNumContainers();
    }
    catch (const web::json::json_exception& excep) {
        SPDLOG_ERROR("ERROR Parsing JSON file: {}", excep.what());
        return __ERROR__;
    }
    catch(const std::runtime_error& re) {
        // speciffic handling for runtime_error
        SPDLOG_ERROR("Runtime error: {}", re.what());
        return __ERROR__;
    }
    catch(const std::exception& ex) {
        // speciffic handling for all exceptions extending std::exception, except
        // std::runtime_error which is handled explicitly
        SPDLOG_ERROR("Error occurred: {}", ex.what());
        return __ERROR__;
    }
    catch(...) {
        // catch any other errors (that we have no information about)
        SPDLOG_ERROR("Unknown failure occurred. Possible memory corruption");
        return __ERROR__;
    }

    return 0;
}

void ec::Facade::JSONFacade::json::createJSONPodDef(const std::string &app_name, const std::string &app_image, const std::string &pod_name, std::string &response) {
    // Create a JSON object (the pod)
    web::json::value pod;
    pod[U("kind")] = web::json::value::string(U("Pod"));
    pod[U("apiVersion")] = web::json::value::string(U("v1"));

//    std::string pod_name_ = web::json::value::string(U())]]

    // Create a JSON object (the metadata)
    web::json::value metadata;
    metadata[U("namespace")] = web::json::value::string(U("default"));
    metadata[U("name")] = web::json::value::string(U(pod_name));

    // Create a JSON object (the metadata label)
    web::json::value metadata_label;
    metadata_label[U("name")] = web::json::value::string(U(pod_name));

    metadata[U("labels")] = metadata_label;

    pod[U("metadata")] = metadata;

    // Now we worry about the specs..
    web::json::value cont1;
    cont1[U("name")] = web::json::value::string(U(pod_name));
    // Default image is nginx
    cont1[U("image")] = web::json::value::string(U(app_image));

    set_pod_limits(cont1);

    web::json::value cont1_port;
    cont1_port[U("containerPort")] = web::json::value::number(U(80));

    web::json::value ports;
    ports[0] = cont1_port;
    cont1[U("ports")] = ports;

    // Create the items array
    web::json::value containers;
    containers[0] = cont1;

    web::json::value cont;
    cont[U("containers")] = containers;

    pod[U("spec")] = cont;

    // Write the current JSON value to a stream with the native platform character width
    utility::stringstream_t stream;
    pod.serialize(stream);
    response = stream.str();
}

void ec::Facade::JSONFacade::json::set_pod_limits(web::json::value &cont) {
    web::json::value requests, limits, resources;
    auto cpu_ = _specs.find("cpu")->second;
    auto cpu_req = std::to_string(cpu_) + "m";
    auto cpu_lim = std::to_string(cpu_) + "m";
//    auto cpu = std::to_string(_specs.find("cpu")->second) + "m";
    auto mem = std::to_string(_specs.find("mem")->second) + "Mi";

    //std::cout << "cpu: " << cpu_req << std::endl;
    //std::cout << "mem: " << mem << std::endl;
//    requests[U("cpu")] = web::json::value::string(U(cpu_req));
    requests[U("memory")] = web::json::value::string(U(mem));

    limits[U("cpu")] = web::json::value::string(U(cpu_lim));
    limits[U("memory")] = web::json::value::string(U(mem));

    resources[U("requests")] = requests;
    resources[U("limits")] = limits;

    cont[U("resources")] = resources;
}

void ec::Facade::JSONFacade::json::postJSONRequest(const std::string &url, const std::string &jsonRequest, std::string &jsonResp) {
    web::json::value jsonRequestVal = web::json::value::parse(jsonRequest);
    web::json::value json_return;

    web::http::client::http_client client(url);
    client.request(web::http::methods::POST, U("/"), jsonRequestVal)
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
        catch(...) {
            std::cerr << "Unknown failure occurred during JSON Post Request. " << std::endl;
        } 
    }).wait();
    jsonResp = json_return.serialize();
    // return json_return.serialize();
}

void ec::Facade::JSONFacade::json::getJSONRequest(const std::string &urlRequest, std::string &jsonResp) {

    web::json::value json_return;
    web::http::client::http_client client(urlRequest);
    client.request(web::http::methods::GET, U("/"))
    .then([](const web::http::http_response& response) {
        if (response.status_code() == web::http::status_codes::OK) {
            return response.extract_json();
        } else {
            std::cout << "status code not OK! it is: " << response.status_code() << std::endl;
            return pplx::task_from_result(web::json::value());
        }
    })
    .then([&json_return](const pplx::task<web::json::value>& task) {
        try {
            json_return = task.get();
        }
        catch (const web::http::http_exception& e) {                    
            std::cout << "error " << e.what() << std::endl;
            json_return = web::json::value::null();	
        }
    })
    .wait();
    if (json_return.is_null()) {
        jsonResp = "";
    } else {
        jsonResp = json_return.serialize();
    }
    // return json_return.serialize();
}

void ec::Facade::JSONFacade::json::getStringResponseFromURL(const std::string &urlRequest, std::string &jsonResp) {

    web::http::client::http_client client(urlRequest);
    client.request(web::http::methods::GET)
    .then([](const web::http::http_response& response) {
        return response.extract_string(); 
    })
    .then([&jsonResp](const pplx::task<std::string>& task) {
        try {
            jsonResp = task.get();
        }
        catch (const web::http::http_exception& e) {                    
            std::cout << "error " << e.what() << std::endl;
        }
    })
    .wait();
}

void ec::Facade::JSONFacade::json::getNodesFromResponse(const std::string &jsonResp, std::vector<std::string> &resultNodes) {
    web::json::value jsonResponse = web::json::value::parse(jsonResp);
    // Todo - check what happens if 2 nodes have the same container name:
    auto node_array = jsonResponse.at(U("spec")).at(U("nodeName")).as_string();
    resultNodes.push_back(node_array);
}

void ec::Facade::JSONFacade::json::getNodeIPFromResponse(const std::string &jsonResp, std::string &tmp_ip) {
    web::json::value jsonResponse = web::json::value::parse(jsonResp);
    auto jsonval = jsonResponse.at(U("status")).at(U("addresses")).as_array();
    for (auto const &i : jsonval) {
        if (i.at(U("type")).as_string() == "InternalIP") {
            tmp_ip = i.at(U("address")).as_string();
        }
    }
}

void ec::Facade::JSONFacade::json::getPodStatusFromResponse(const std::string &jsonResp, std::string &tmp_ip) {
    web::json::value jsonResponse = web::json::value::parse(jsonResp);
    tmp_ip = jsonResponse.at(U("status")).at(U("phase")).as_string();
}

uint64_t ec::Facade::JSONFacade::json::parseCAdvisorResponseSpecs(const std::string &jsonResp, const std::string &resource, const std::string &type){
    web::json::value jsonResponse = web::json::value::parse(jsonResp);
    const utility::string_t &kubePodName = jsonResponse.as_object().cbegin()->first;
    const web::json::value &kubePodSpecs = jsonResponse.as_object().cbegin()->second;
    const web::json::object &k = kubePodSpecs.as_object();
    return k.at("spec").at(resource).at(type).as_number().to_uint64();
}

uint64_t ec::Facade::JSONFacade::json::parseCAdvisorCPUResponseStats(const std::string &jsonResp, const std::string &resource, const std::string &type){
    web::json::value jsonResponse = web::json::value::parse(jsonResp);
    const utility::string_t &kubePodName = jsonResponse.as_object().cbegin()->first;
    const web::json::value &kubePodStats = jsonResponse.as_object().cbegin()->second;
    const web::json::object &k = kubePodStats.as_object();
    const auto len = k.at("stats").as_array().size();
    const auto &last = k.at("stats").as_array().at(len-1).as_object();
    return last.at(resource).at("cfs").at(type).as_number().to_uint64();
}

uint64_t ec::Facade::JSONFacade::json::parseCAdvisorResponseStats(const std::string &jsonResp, const std::string &resource, const std::string &type){
    web::json::value jsonResponse = web::json::value::parse(jsonResp);
    const utility::string_t &kubePodName = jsonResponse.as_object().cbegin()->first;
    const web::json::value &kubePodStats = jsonResponse.as_object().cbegin()->second;
    const web::json::object &k = kubePodStats.as_object();
    const auto len = k.at("stats").as_array().size();
    const auto &last = k.at("stats").as_array().at(len-1).as_object();
    return last.at(resource).at(type).as_number().to_uint64();
}

uint64_t ec::Facade::JSONFacade::json::parseCAdvisorMachineStats(const std::string &jsonResp, const std::string &type) {
    web::json::value jsonResponse = web::json::value::parse(jsonResp);
    const web::json::object &machineSpecs = jsonResponse.as_object();
    return machineSpecs.at(type).as_number().to_uint64();
}


uint64_t ec::Facade::JSONFacade::json::get_mem() {
    auto itr = _specs.find("mem");
    if(itr != _specs.end()) {
        return itr->second;
    }
    return 0;
}

uint64_t ec::Facade::JSONFacade::json::get_cpu() {
    auto itr = _specs.find("cpu");
    if(itr != _specs.end()) {
        return itr->second;
    }
    return 0;
}

uint64_t ec::Facade::JSONFacade::json::get_ports() {
    auto itr = _specs.find("ports");
    if(itr != _specs.end()) {
        return itr->second;
    }
    return 0;
}

uint64_t ec::Facade::JSONFacade::json::get_net() {
    auto itr = _specs.find("net");
    if(itr != _specs.end()) {
        return itr->second;
    }
    return 0;
}

