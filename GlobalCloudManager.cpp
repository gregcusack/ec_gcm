//
// Created by greg on 9/12/19.
//

#include "GlobalCloudManager.h"

ec::GlobalCloudManager::GlobalCloudManager()
    : gcm_ip(ec::ip4_addr::from_string("127.0.0.1")), gcm_port(4444), ec_counter(1) {}

ec::GlobalCloudManager::GlobalCloudManager(std::string ip_addr, uint16_t port)
    : gcm_ip(ec::ip4_addr::from_string(std::move(ip_addr))), gcm_port(port), ec_counter(1) {}

uint32_t ec::GlobalCloudManager::create_ec() {
    if(ecs.find(ec_counter) != ecs.end()) {
        std::cout << "ERROR: Error allocating new Manager. Manager IDs not correct" << std::endl;
        return 0;
    }

    auto *ec = new ec::ElasticContainer(ec_counter, gcm_ip);
    ecs.insert({ec_counter, ec});

//    eclock.lock();
    ec_counter++;
//    eclock.unlock();
    return ec->get_ec_id();
}

ec::ElasticContainer *ec::GlobalCloudManager::get_ec(uint32_t ec_id) {
    auto itr = ecs.find(ec_id);
    if(itr == ecs.end()) {
        std::cout << "ERROR: No EC with ec_id: " << ec_id << ". Exiting...." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return itr->second;
}
