#include "../ECAPI.h"
#include "../om.h"
#include "../Agents/Agent.h"

int main() {
    /*
     1. Get Agent Clients
     2. Instantiate ECAPI
     3. Create a distributed container
          - There are multiple things happening here:
              - k8s Pod definition (i.e. in JSON) is being generated and sent via the k8 REST API
                  - Requires k8 proxy to be running on localhost, port 8000
              - All the agents running in the nodes are instructed to call "sys_connect" 
                with a specified container name
                    - Next step: automate this via a registrator + etcd backend
              - Agents respond with the status of calling sys_connect
              - Individual containers communicate with GCM server to register themselves as part of the 
                distributed container group
    */

    // 1. Get/Create Agent Clients
    std::vector<Agent*> agents;
    std::vector<std::string>    agent_ips{"192.168.6.8"};
    for(const auto &i : _agent_ips) {
        agents.emplace_back(new Agent(i));
    }

    // 2. Instantiate ECAPI 
    auto *ec = new ec::ECAPI::ECAPI(1, agents);

    // 3. Create a distributed container
    int new_ec_status = ec->create_ec();
    if (new_ec_status != 0){
        std::cout << "Error in creating ec" << std::endl;
        return -1;
    }
    std::cout << "Container status: " << new_ec_status << std::endl;
}