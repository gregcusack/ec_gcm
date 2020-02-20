#include <iostream>
#include <fstream>

#include "GlobalCloudManager.h"
#include "ElasticContainer.h"
#include "SubContainer.h"
#include "types/msg.h"
#include "jsonSDK/include/JSONFacade.h"


#define GCM_PORT        8888             //Not sure if we need a port here tbh

int main(int argc, char* argv[]){

    // Check the number of parameters
    if (argc < 2) {
        // Tell the user how to run the program
        std::cerr << "Usage: " << argv[0] << " <path-to-json-file>" << std::endl;
        return 1;
    }
    std::string jsonFile = argv[1];
    std::vector<std::string>    agent_ips{"128.105.144.137"};//), "127.0.0.1", "127.0.0.1"};
    std::vector<uint16_t>       server_ports{4444};

    auto *gcm = new ec::GlobalCloudManager("128.105.144.138", GCM_PORT, agent_ips, server_ports);

    JSONFacade jsonFacade;
    jsonFacade.parseFile(jsonFile);
    auto app_name = jsonFacade.getAppName();
    auto app_images = jsonFacade.getAppImages(); 
    
    for(const auto &i : server_ports) {
        gcm->create_server();
    }
    std::cout << "[dbg] num servers: " << gcm->get_servers().size() << std::endl;

    gcm->run(app_name, app_images);

    delete gcm;


    return 0;
}
