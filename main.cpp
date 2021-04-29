#include <iostream>
#include <fstream>

#include "GlobalControlManager.h"
#include "ElasticContainer.h"
#include "SubContainer.h"
#include "Agents/AgentClientDB.h"
#include "types/ports.h"

#ifndef SPDLOG_ACTIVE_LEVEL
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#endif

#include "spdlog/spdlog.h"
#define GCM_PORT        8888             //Not sure if we need a port here tbh

/* LOG LEVELS
 *  SPDLOG_TRACE("Some trace message with param {}", 42);
 *  SPDLOG_DEBUG("Some debug message");
 *  SPDLOG_INFO("SUHHH");
 *  SPDLOG_WARN("WARN");
 *  SPDLOG_ERROR("ERROR");
 *  SPDLOG_CRITICAL("CRITICAL");
 */

ec::AgentClientDB* ec::AgentClientDB::agent_clients_db_instance = nullptr;

int main(int argc, char* argv[]){
#if(SPDLOG_ACTIVE_LEVEL == SPDLOG_LEVEL_DEBUG)
    spdlog::set_level(spdlog::level::debug);
#elif(SPDLOG_ACTIVE_LEVEL == SPDLOG_LEVEL_TRACE)
    spdlog::set_level(spdlog::level::trace);
#endif

    // Check the number of parameters
    if (argc < 2) {
        // Tell the user how to run the program
        SPDLOG_ERROR("Usage: {} <path-to-json-file>", argv[0]);
        return 1;
    }
    const std::string &jsonFile = argv[1];

    int status;
    ec::Facade::JSONFacade::json jsonFacade;
    status = jsonFacade.parseFile(jsonFile);
    if (status != 0) {
        SPDLOG_ERROR("[dbg] Error in parsing file.. exiting");
        SPDLOG_ERROR("[status code]: {}", status);
        return 1;
    }
    auto app_name = jsonFacade.getAppName();
    auto agent_ips = jsonFacade.getAgentIPs();
    auto gcm_ip = jsonFacade.getGCMIP();
    
//    ec::ports_t app1_ports(4444, 4447);
    ec::ports_t app1_ports(5000, 6000);
    ec::ports_t app2_ports(5001, 6001);

//    std::vector<ec::ports_t> controller_ports{app1_ports};
    std::vector<ec::ports_t> controller_ports{app1_ports, app2_ports};

    auto *gcm = new ec::GlobalControlManager(gcm_ip, GCM_PORT, agent_ips, controller_ports);

    for(const auto &i : controller_ports) {
        gcm->create_manager();
    }
    SPDLOG_INFO("[dbg] num managers: {}", gcm->get_managers().size());

    //    Create AgentClients
    if(!gcm->init_agent_connections()) {
        SPDLOG_CRITICAL("[ERROR Server] not all agents connected to server_id: {}! Exiting...");
        exit(EXIT_FAILURE);
    }
    
    gcm->run(app_name, gcm_ip);

    delete gcm;

    return 0;
}
