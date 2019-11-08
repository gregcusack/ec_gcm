#include <iostream>

#include "GlobalCloudManager.h"
#include "ElasticContainer.h"
#include "SubContainer.h"
#include "types/msg.h"


#define MANAGER_ID      23
#define CONTAINER_ID    1
#define SERVER_IP       2130706433      //127.0.0.1
#define GCM_PORT        8888             //Not sure if we need a port here tbh
#define SERVER_PORT     4444

int main(){
    std::vector<std::string>    agent_ips{"127.0.0.1"};//), "127.0.0.1", "127.0.0.1"};
    std::vector<uint16_t>       server_ports{4444};

    auto *gcm = new ec::GlobalCloudManager("127.0.0.1", GCM_PORT, agent_ips, server_ports);

    for(const auto &i : server_ports) {
        gcm->create_server();
    }
    std::cout << "num servers: " << gcm->get_servers().size() << std::endl;

    gcm->run();

    delete gcm;


    return 0;
}
