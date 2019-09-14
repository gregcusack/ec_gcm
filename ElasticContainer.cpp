//
// Created by greg on 9/12/19.
//

#include "ElasticContainer.h"

ec::ElasticContainer::ElasticContainer(uint32_t _ec_id, ip4_addr _ip_address)
    : ec_id(_ec_id), ip_address(_ip_address) {

    manager = nullptr;
    _mem = memory();
    _cpu = cpu();
}

void ec::ElasticContainer::create_manager(uint16_t port) {
    manager = new Manager(ec_id, ip_address, port);       //pass in gcm_ip for now
}

ec::Manager *ec::ElasticContainer::get_manager() {
    if(manager == nullptr) {
        std::cout << "[ERROR]: Must create manager before accessing it" << std::endl;
        exit(EXIT_FAILURE);
    }
    return manager;
}

//void ec::ElasticContainer::set_period(int64_t _period) {
//    _cpu.period = _period;
//    if(manager != nullptr) {
//        manager
//    }
//}
