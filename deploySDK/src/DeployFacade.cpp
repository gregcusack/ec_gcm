#include "../include/DeployFacade.h"

int k8Facade::deployPod(const std::string jsonString) {
    std::string res;
    JSONFacade jsonFacade;

    // Assumes that there's a k8s proxy running on localhost, port 8000
    res = jsonFacade.postJSONRequest("http://localhost:8000/api/v1/namespaces/default/pods", jsonString);
    
    // Todo: Check whether there's a way we can check if the pod hasn't been scheduled 
    //       at this point
    return 0;
}

std::vector<std::string> k8Facade::getNodesWithPod(std::string podName) {
    std::vector<std::string> resultNodes;
    JSONFacade jsonFacade;

    // Assumes that there's a k8s proxy running on localhost, port 8000
    std::string res;
    res = jsonFacade.getJSONRequest("http://localhost:8000/api/v1/namespaces/default/pods/" + podName);

    resultNodes = jsonFacade.getNodesFromResponse(res);
    return resultNodes;
}

std::vector<std::string> k8Facade::getNodeIPs(std::vector<std::string> nodeNames) {
    std::vector<std::string> resultIPs;
    JSONFacade jsonFacade;

    // Assumes that there's a k8s proxy running on localhost, port 8000
    std::string res;
    std::string tmp_ip;
    for(const auto node: nodeNames) {
        res = jsonFacade.getJSONRequest("http://localhost:8000/api/v1/nodes/" + node);
        tmp_ip = jsonFacade.getNodeIPFromResponse(res);
        if(tmp_ip.empty()) {
            continue;
        }
        resultIPs.push_back(jsonFacade.getNodeIPFromResponse(res));
    }
    return resultIPs;
}