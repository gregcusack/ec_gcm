//
// Created by greg on 9/12/19.
//

#include "Manager.h"

//ec::Manager::Manager(uint32_t _ec_id, ip4_addr _ip_address, uint16_t _port, std::vector<Agent *> &_agents)
//    : manager_id(_ec_id), ip_address(_ip_address), port(_port), agents(_agents) {
//
////    server = nullptr;
//    _ec = nullptr;
//}

ec::Manager::Manager(uint32_t _ec_id, std::vector<Agent *> &_agents)
    : manager_id(_ec_id), agents(_agents) {

    _ec = new ElasticContainer(manager_id, agents);
}


void ec::Manager::create_ec() {
    _ec = new ElasticContainer(manager_id, agents);       //pass in gcm_ip for now
}

//void ec::Manager::create_server() {
//    server = new Server(ip_address, port);
//}

//void ec::Manager::connect_server_and_ec() {
//    _ec->set_server(server);
//    server->set_ec(_ec);
//}

const ec::ElasticContainer& ec::Manager::get_elastic_container() const {
    if(_ec == nullptr) {
        std::cout << "[ERROR]: Must create _ec before accessing it" << std::endl;
        exit(EXIT_FAILURE);
    }
    return *_ec;
}

//void ec::Manager::build_manager_handler() {
//    create_ec();
////    create_server();
////    connect_server_and_ec();
//}

ec::Manager::~Manager() {
    delete _ec;
//    delete server;
}

int ec::Manager::handle_add_cgroup_to_ec(ec::msg_t *res, uint32_t cgroup_id, const uint32_t ip, int fd) {
    return _ec->add_cgroup_to_ec(res, cgroup_id, ip, fd);
}




//_ec::Manager::Manager(uint32_t _ec_id, _ec::ip4_addr _ip_address, std::vector<GlobalCloudManager::agent *> &_agents)
//    : manager_id(_ec_id), ip_address(_ip_address), agents(&_agents) {
//
//    server = nullptr;
//    _ec = nullptr;
//    _mem = memory();
//    _cpu = cpu();
//
//}


//void _ec::Manager::set_period(int64_t _period) {
//    _cpu.period = _period;
//    if(_ec != nullptr) {
//        _ec
//    }
//}
