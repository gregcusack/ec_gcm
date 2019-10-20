//
// Created by greg on 9/12/19.
//

#include "ElasticContainer.h"

ec::ElasticContainer::ElasticContainer(uint32_t _ec_id, ip4_addr _ip_address, std::vector<Agent *> &_agents)
    : ec_id(_ec_id), ip_address(_ip_address), agents(_agents) {

    server = nullptr;
    manager = nullptr;
    _mem = memory();
    _cpu = cpu();
}

void ec::ElasticContainer::create_manager() {
    manager = new Manager(ec_id, agents, _cpu.quota, _cpu.slice_size,
                          _mem.memory_limit, _mem.slice_size);       //pass in gcm_ip for now
}

void ec::ElasticContainer::create_server(uint16_t _port) {
    server = new Server(ip_address, _port);
}

void ec::ElasticContainer::connect_server_and_manager() {
    manager->set_server(server);
    server->set_manager(manager);
}

ec::Manager *ec::ElasticContainer::get_manager() {
    if(manager == nullptr) {
        std::cout << "[ERROR]: Must create manager before accessing it" << std::endl;
        exit(EXIT_FAILURE);
    }
    return manager;
}

void ec::ElasticContainer::build_ec_handler(uint16_t _port) {
    create_manager();
    create_server(_port);
    connect_server_and_manager();
}

//ec::ElasticContainer::ElasticContainer(uint32_t _ec_id, ec::ip4_addr _ip_address, std::vector<GlobalCloudManager::agent *> &_agents)
//    : ec_id(_ec_id), ip_address(_ip_address), agents(&_agents) {
//
//    server = nullptr;
//    manager = nullptr;
//    _mem = memory();
//    _cpu = cpu();
//
//}


//void ec::ElasticContainer::set_period(int64_t _period) {
//    _cpu.period = _period;
//    if(manager != nullptr) {
//        manager
//    }
//}
