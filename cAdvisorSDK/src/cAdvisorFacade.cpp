#include "../include/cAdvisorFacade.h"

uint64_t ec::Facade::MonitorFacade::CAdvisor::getContCPULimit(const std::string agent_ip, const std::string docker_container_id) {
    std::string res;
    ec::Facade::JSONFacade::json jsonFacade;
    jsonFacade.getJSONRequest("http://" + agent_ip + ":8080/api/v1.2/docker/" + docker_container_id, res);
    std::cout << res << std::endl;
}


uint64_t ec::Facade::MonitorFacade::CAdvisor::getContMemLimit(const int agent_sock, const std::string docker_container_id) {
    std::string json_resp;
    uint64_t res;

    ec::Facade::ProtoBufFacade::ProtoBuf protoFacade;
    msg_struct::ECMessage tx_msg;
    tx_msg.set_req_type(5);
    tx_msg.set_client_ip(docker_container_id);
    tx_msg.set_payload_string("max_usage");
    res = protoFacade.sendMessage(agent_sock, tx_msg);
    if(res < 0) {
        std::cout << "[ERROR]: getContMemLimit: Error in writing to agent_clients socket. " << std::endl;
    }
    msg_struct::ECMessage rx_msg;
    protoFacade.recvMessage(agent_sock, rx_msg);
    res = rx_msg.rsrc_amnt();
    return res;
}

// // Todo: This can be moved to another Facade but is it worth it?
// std::string CAdvisor::exec(string cmd) {
//     string data;
//     FILE * stream;
//     const int max_buffer = 256;
//     char buffer[max_buffer];
//     cmd.append(" 2>&1");

//     stream = popen(cmd.c_str(), "r");
//     if (stream) {
//     while (!feof(stream))
//     if (fgets(buffer, max_buffer, stream) != NULL) data.append(buffer);
//     pclose(stream);
//     }
//     return data;
// }