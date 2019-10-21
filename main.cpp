#include <iostream>

#include "GlobalCloudManager.h"
#include "Manager.h"
#include "SubContainer.h"
#include "types/msg.h"
#include <sys/wait.h>


#define MANAGER_ID      23
#define CONTAINER_ID    1
#define SERVER_IP       2130706433      //127.0.0.1
#define GCM_PORT        8888             //Not sure if we need a port here tbh
#define SERVER_PORT     4444

int main(){
    std::vector<std::string>    agent_ips{"127.0.0.1"};//), "127.0.0.1", "127.0.0.1"};
    std::vector<uint16_t>       ec_ports{4444};

//    auto *gcm = new ec::GlobalCloudManager("128.138.244.104", GCM_PORT);
    auto *gcm = new ec::GlobalCloudManager("127.0.0.1", GCM_PORT, agent_ips, ec_ports);


    for(const auto &i : ec_ports) {
        gcm->create_ec();
    }
    std::cout << "num ecs: " << gcm->get_ecs().size() << std::endl;

    for(const auto &ec : gcm->get_ecs()) {
        if(fork() == 0) {
            std::cout << "[child] pid: " << getpid() << ", [parent] pid: " <<  getppid() << std::endl;
            std::cout << "ec_id: " << ec.second->get_ec_id() << std::endl;
            ec.second->build_ec_handler();
            auto *m = ec.second->get_manager();
            auto *s = m->get_server();
            s->initialize_server();
            s->serve();
        }
        else {
            continue;
        }
        break;
    }
    for(const auto &i : gcm->get_ecs()) {
        wait(nullptr);
    }

//    delete agents;
    delete gcm;


    return 0;
}
