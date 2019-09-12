//
// Created by greg on 9/12/19.
//

#include "GlobalCloudManager.h"

ec::GlobalCloudManager::GlobalCloudManager()
    : gcm_ip(ec::ip4_addr::from_string("127.0.0.1")), gcm_port(4444), manager_counter(1) {}

ec::GlobalCloudManager::GlobalCloudManager(std::string ip_addr, uint16_t port)
    : gcm_ip(ec::ip4_addr::from_string(std::move(ip_addr))), gcm_port(port), manager_counter(1) {}

uint32_t ec::GlobalCloudManager::create_new_manager() {
    if(managers.find(manager_counter) != managers.end()) {
        std::cout << "ERROR: Error allocating new Manager. Manager IDs not correct" << std::endl;
        return 0;
    }
    auto *m = new ec::Manager(manager_counter);
    managers.insert({manager_counter, m});
    manager_counter++;
    return m->get_ec_id();
}

ec::Manager &ec::GlobalCloudManager::get_manager(uint32_t ec_id) {
    auto itr = managers.find(ec_id);
    if(itr == managers.end()) {
        std::cout << "ERROR: No EC with ec_id: " << ec_id << ". Exiting...." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return *itr->second;

//    return *managers.at(ec_id);
//    if(managers.find(ec_id) == managers.end()) {
//        return nullptr_t;
//    }
//    return
}
