#ifndef DEPLOY_FACADE_H
#define DEPLOY_FACADE_H

#include <iostream>
#include <fstream>
#include <sstream>
#include "../../jsonSDK/include/JSONFacade.h"

namespace ec {
    namespace Facade {
        namespace DeployFacade {
            class k8Facade {
                public:
                    static int deployPod(const std::string &jsonString);
                    static void getNodesWithPod(const std::string &podName, std::vector<std::string> &resultNodes);
                    static void getNodeIPs(const std::vector<std::string> &nodeNames, std::vector<std::string> &nodeIPs);
            };
        }
    }
}

// Needs to be implemented when I do research about
// how docker swarm deploys containers
namespace ec {
    namespace Facade {
        namespace DeployFacade {
            class swarmFacade {};   
        }
    }
}

namespace ec {
    namespace Facade {
        namespace DeployFacade {
            class Deploy {
            public:
                int deployContainers(const std::string &jsonString) {
                    // Todo: This is where we can check whether to use k8s 
                    // or Swarm to deploy containers
                    int status;
                    status = _k8sDeployment.deployPod(jsonString);
                    _deploymentType = K8s;
                    return status;
                }

                void getNodesWithContainer(const std::string &containerName, std::vector<std::string> &resultNodes) {
                    if(_deploymentType == K8s) {
                        _k8sDeployment.getNodesWithPod(containerName, resultNodes);
                    } 
                }

                void getNodeIPs(const std::vector<std::string> &nodeNames, std::vector<std::string> &nodeIPs) {
                    if(_deploymentType == K8s) {
                        _k8sDeployment.getNodeIPs(nodeNames, nodeIPs);
                    } 
                }

            private:
                ec::Facade::DeployFacade::k8Facade _k8sDeployment;
                ec::Facade::DeployFacade::swarmFacade _swarmDeployment;
                enum DeploymentType {
                    K8s, Swarm
                };
                int _deploymentType;
            };
        }
    }
}

#endif //DEPLOY_FACADE_H
