#include <iostream>

#include "GlobalCloudManager.h"
#include "ElasticContainer.h"
#include "SubContainer.h"
#include "types/msg.h"


#define GCM_PORT        8888             //Not sure if we need a port here tbh

int main(){
    std::vector<std::string>    agent_ips{"192.168.6.10"};//), "127.0.0.1", "127.0.0.1"};
    std::vector<uint16_t>       server_ports{4444};

    auto *gcm = new ec::GlobalCloudManager("192.168.6.8", GCM_PORT, agent_ips, server_ports);

    for(const auto &i : server_ports) {
        gcm->create_server();
    }
    std::cout << "num servers: " << gcm->get_servers().size() << std::endl;

    gcm->run();

    delete gcm;


    return 0;
}
