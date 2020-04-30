#include <iostream>
#include <fstream>

#include "GlobalControlManager.h"
#include "ElasticContainer.h"
#include "SubContainer.h"
#include "types/msg.h"
#include "Agents/AgentClientDB.h"
#include <cpprest/http_client.h>
#include <cpprest/json.h> // JSON library

#define GCM_PORT        8888             //Not sure if we need a port here tbh
ec::AgentClientDB* ec::AgentClientDB::agent_clients_db_instance = 0;

int main(int argc, char* argv[]){

    // Check the number of parameters
    if (argc < 2) {
        // Tell the user how to run the program
        std::cerr << "Usage: " << argv[0] << " <path-to-json-file>" << std::endl;
        return 1;
    }
    const std::string &jsonFile = argv[1];
    
    int status;
    ec::Facade::JSONFacade::json jsonFacade;
    status = jsonFacade.parseFile(jsonFile);
    if (status != 0) {
        std::cout << "[dbg] Error in parsing file.. exiting" << std::endl;
        std::cout << "[status code]: " << status << std::endl;
        return 1;
    }
    auto app_name = jsonFacade.getAppName();
    auto app_images = jsonFacade.getAppImages(); 
    auto agent_ips = jsonFacade.getAgentIPs();
    auto pod_names = jsonFacade.getPodNames();
    auto gcm_ip = jsonFacade.getGCMIP();
    
    std::vector<uint16_t>       server_ports{4444};

    auto *gcm = new ec::GlobalControlManager(gcm_ip, GCM_PORT, agent_ips, server_ports);
    
    for(const auto &i : server_ports) {
        gcm->create_manager();
    }
    std::cout << "[dbg] num managers: " << gcm->get_managers().size() << std::endl;
    
    gcm->run(app_name, app_images, pod_names, gcm_ip);

    delete gcm;

    return 0;
}
