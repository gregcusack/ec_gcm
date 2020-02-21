#ifndef DEPLOY_FACADE_H
#define DEPLOY_FACADE_H

#include <iostream>
#include <fstream>
#include <sstream>
#include "../../jsonSDK/include/JSONFacade.h"

class k8Facade {
    public:
        int deployPod(const std::string jsonString);
        std::vector<std::string> getNodesWithPod(std::string podName);
        std::vector<std::string> getNodeIPs(std::vector<std::string> nodeNames);
};

// Needs to be implemented when I do research about
// how docker swarm deploys containers
class swarmFacade {};

class DeployFacade {
    public:
        DeployFacade() {}
        int deployContainers(const std::string jsonString) {
            // Todo: This is where we can check whether to use k8s 
            // or Swarm to deploy containers
            int status;
            status = _k8sDeployment.deployPod(jsonString);
            _deploymentType = K8s;
            return status;
        }

        std::vector<std::string> getNodesWithContainer(std::string containerName) {
            if(_deploymentType == K8s) {
                return _k8sDeployment.getNodesWithPod(containerName);
            } 
        }

        std::vector<std::string> getNodeIPs(std::vector<std::string> nodeNames) {
            if(_deploymentType == K8s) {
                return _k8sDeployment.getNodeIPs(nodeNames);
            } 
        }

    private:
        k8Facade _k8sDeployment;
        swarmFacade _swarmDeployment;
        enum DeploymentType {
            K8s, Swarm
        };
        int _deploymentType;
};

#endif //DEPLOY_FACADE_H
