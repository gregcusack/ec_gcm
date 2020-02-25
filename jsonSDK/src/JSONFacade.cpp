#include "../include/JSONFacade.h"

void JSONFacade::parseAppName() {
    if(_val.is_null()) {
        return;
    }
   _app_name = _val.at(U("name")).as_string();
}

void JSONFacade::parseAppImages() {
    if(_val.is_null()) {
        return;
    }
    auto app_images = _val.at(U("images")).as_array();
    for(const auto &i : app_images) {
        _app_images.push_back(i.as_string());
    }
}

void JSONFacade::parseIPAddresses() {
    if(_val.is_null()) {
        return;
    }
    auto agent_ips = _val.at(U("agentIPs")).as_array();
    for(const auto &i : agent_ips) {
        _agent_ips.push_back(i.as_string());
    }
}

void JSONFacade::parseGCMIPAddress() {
    if(_val.is_null()) {
        return;
    }
   _gcm_ip = _val.at(U("gcmIP")).as_string();
}

int JSONFacade::parseFile(const std::string fileName) {
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
        return 0;
    }
    catch (const web::json::json_exception& excep) {
        std::cout << "ERROR Parsing JSON file: " << excep.what() << std::endl;
        return 1;
    }
    catch(const std::runtime_error& re) {
        // speciffic handling for runtime_error
        std::cerr << "Runtime error: " << re.what() << std::endl;
        return 2;
    }
    catch(const std::exception& ex) {
        // speciffic handling for all exceptions extending std::exception, except
        // std::runtime_error which is handled explicitly
        std::cerr << "Error occurred: " << ex.what() << std::endl;
        return 3;
    }
    catch(...) {
        // catch any other errors (that we have no information about)
        std::cerr << "Unknown failure occurred. Possible memory corruption" << std::endl;
        return 4;
    }
}

std::string JSONFacade::createJSONPodDef(const std::string app_name, const std::string app_image) {
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
    return stream.str();          
}

std::string JSONFacade::postJSONRequest(const std::string url, const std::string jsonRequest) {
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
    return json_return.serialize();
}

std::string JSONFacade::getJSONRequest(const std::string urlRequest) {

    web::json::value json_return;
    web::http::client::http_client client(urlRequest);
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
    return json_return.serialize();
}

std::vector<std::string> JSONFacade::getNodesFromResponse(const std::string jsonResp) {
    std::vector<std::string> output;
    web::json::value jsonResponse = web::json::value::parse(jsonResp);
    
    // Todo - check what happens if 2 nodes have the same container name:
    auto node_array = jsonResponse.at(U("spec")).at(U("nodeName")).as_string();
    output.push_back(node_array);
    return output;
}

std::string JSONFacade::getNodeIPFromResponse(const std::string jsonResp) {
    web::json::value jsonResponse = web::json::value::parse(jsonResp);
    auto jsonval = jsonResponse.at(U("status")).at(U("addresses")).as_array();
    for (int i = 0; i < jsonval.size(); i++) {
        if (jsonval[i].at(U("type")).as_string() == "InternalIP") {
            return jsonval[i].at(U("address")).as_string();
        }
    }
    return NULL;
}

uint64_t JSONFacade::parseCAdvisorResponseLimits(const std::string jsonResp, const std::string type){
    web::json::value jsonResponse = web::json::value::parse(jsonResp);
    
    if(jsonResponse.is_null()) {
        std::cout << "Null";
    }
    if(jsonResponse.is_number()) {
        std::cout << "number";
    }
    if(jsonResponse.is_integer()) {
        std::cout << "int";
    }
    if(jsonResponse.is_double()) {
        std::cout << "double";
    }
    if(jsonResponse.is_boolean()) {
        std::cout << "bool";
    }
    if(jsonResponse.is_string ()) {
        std::cout << "string";
    }
    if(jsonResponse.is_array()) {
        std::cout << "array";
    }
    if(jsonResponse.is_object()) {
        std::cout << "objc";
    }
    
    //auto DataArray = jsonResponse.as_string();
    // for (auto Iter = DataArray.begin(); Iter != DataArray.end(); ++Iter)
    // {
    //     auto& data = *Iter;
    //     //auto dataObj = data.second.as_object();

    //     // for (auto iterInner = dataObj.cbegin(); iterInner != dataObj.cend(); ++iterInner)
    //     // {
    //     //     auto &propertyName = iterInner->first;
    //     //     auto &propertyValue = iterInner->second;

    //     //     std::cout << "Property: " << propertyName << ", Value: " << 
    //     //     propertyValue << std::endl;
    //     // }
    // }
    // for(web::json::object::iterator itr = jsonval.begin(); itr != jsonval.end(); ++itr) {
    //     // auto jsonval1 = itr->second.as_array();
    //     // auto jsonvalMemArray = jsonval1.at(U("memory")).at(U("max_usage")).as_string();
    //     // std::cout << jsonvalMemArray << std::endl;     
    //     // for (web::json::object::iterator iter = jsonval1.begin(); iter != jsonval1.end(); ++iter) {
    //     //     const utility::string_t &str = itr->first;
    //     //     std::cout << str << std::endl;            
    //     // }   
    // }
    return 1;

}

