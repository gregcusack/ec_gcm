#include "../include/DeployFacade.h"

int ec::Facade::DeployFacade::k8Facade::deployPod(const std::string &jsonString) {
    std::string res;
    // Assumes that there's a k8s proxy running on localhost, port 8000
    ec::Facade::JSONFacade::json::postJSONRequest("http://localhost:8000/api/v1/namespaces/default/pods", jsonString, res);
    
    // Todo: Check whether there's a way we can check if the pod hasn't been scheduled 
    //       at this point
    return 0;
}

void ec::Facade::DeployFacade::k8Facade::getNodesWithPod(const std::string &podName, std::vector<std::string> &resultNodes) {
    ec::Facade::JSONFacade::json jsonFacade;
    // Assumes that there's a k8s proxy running on localhost, port 8000
    std::string res;
    ec::Facade::JSONFacade::json::getJSONRequest("http://localhost:8000/api/v1/namespaces/default/pods/" + podName, res);
    ec::Facade::JSONFacade::json::getNodesFromResponse(res, resultNodes);
}

void ec::Facade::DeployFacade::k8Facade::getNodeIPs(const std::vector<std::string> &nodeNames, std::vector<std::string> &nodeIPs) {
    ec::Facade::JSONFacade::json jsonFacade;
    // Assumes that there's a k8s proxy running on localhost, port 8000
    std::string jsonResp;
    std::string tmp_ip;
    for(const auto &node: nodeNames) {
        ec::Facade::JSONFacade::json::getJSONRequest("http://localhost:8000/api/v1/nodes/" + node, jsonResp);
        ec::Facade::JSONFacade::json::getNodeIPFromResponse(jsonResp, tmp_ip);
        if(tmp_ip.empty()) {
            continue;
        }
        nodeIPs.push_back(tmp_ip);
    }
}