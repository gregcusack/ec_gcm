#include "../include/JSONFacade.h"

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

void ec::Facade::JSONFacade::json::parseGCMIPAddress() {
    if(_val.is_null()) {
        return;
    }
   _gcm_ip = _val.at(U("gcmIP")).as_string();
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
        parseAppImages();
        parseIPAddresses();
        parseGCMIPAddress();
    }
    catch (const web::json::json_exception& excep) {
        std::cout << "ERROR Parsing JSON file: " << excep.what() << std::endl;
        return __ERROR__;
    }
    catch(const std::runtime_error& re) {
        // speciffic handling for runtime_error
        std::cerr << "Runtime error: " << re.what() << std::endl;
        return __ERROR__;
    }
    catch(const std::exception& ex) {
        // speciffic handling for all exceptions extending std::exception, except
        // std::runtime_error which is handled explicitly
        std::cerr << "Error occurred: " << ex.what() << std::endl;
        return __ERROR__;
    }
    catch(...) {
        // catch any other errors (that we have no information about)
        std::cerr << "Unknown failure occurred. Possible memory corruption" << std::endl;
        return __ERROR__;
    }
    return 0;
}

void ec::Facade::JSONFacade::json::createJSONPodDef(const std::string &app_name, const std::string &app_image, std::string &response) {
    std::string pod_name = app_name + "-" + app_image;
    // Create a JSON object (the pod)
    web::json::value pod;
    pod[U("kind")] = web::json::value::string(U("Pod"));
    pod[U("apiVersion")] = web::json::value::string(U("v1"));

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
        return response.extract_json(true); 
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
    jsonResp = json_return.serialize();
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
    for (int i = 0; i < jsonval.size(); i++) {
        if (jsonval[i].at(U("type")).as_string() == "InternalIP") {
            tmp_ip = jsonval[i].at(U("address")).as_string();
        }
    }
}

uint64_t ec::Facade::JSONFacade::json::parseCAdvisorResponseLimits(const std::string &jsonResp, const std::string &resource, const std::string &type){
    web::json::value jsonResponse = web::json::value::parse(jsonResp);
    const utility::string_t &kubePodName = jsonResponse.as_object().cbegin()->first;
    const web::json::value &kubePodStats = jsonResponse.as_object().cbegin()->second;
    //std::cout << kubePodName << std::endl;
    std::vector<uint64_t> tmpVals;
    for(auto iter = kubePodStats.as_object().cbegin(); iter != kubePodStats.as_object().cend(); ++iter) {
        if (iter->first.compare("stats") == 0) {
            const web::json::value &stats_val = iter->second;
            for (auto &iter_val : stats_val.as_array()) {
                for(auto iter_obj = iter_val.as_object().cbegin(); iter_obj != iter_val.as_object().cend(); ++iter_obj) {
                    if (iter_obj->first.compare(resource) == 0) {
                        const web::json::value &mem_stats = iter_obj->second;
                        for(auto iter_mem = mem_stats.as_object().cbegin(); iter_mem != mem_stats.as_object().cend(); ++iter_mem) {
                            if(iter_mem->first.compare(type) == 0) {
                                //std::cout << iter_mem->first << ":" << iter_mem->second.as_number().to_uint64() << std::endl;
                                tmpVals.emplace_back(iter_mem->second.as_number().to_uint64());
                            }
                        }
                    }
                }
            }
        }
    }
    return tmpVals.back();
}
