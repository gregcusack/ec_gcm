#include "../include/cAdvisorFacade.h"

// bool CAdvisor::checkCAdvisor(const std::string agent_ip) {
//     // Assumes docker is installed and cAdvisor is running as a container in the background on port 8080
//     std::string cmd;
//     cmd = 'sudo docker inspect --format="{{.Id}}" cadvisor';
//     if (cmd.size() == 0) {
//         return false;
//     }
//     _cAdvisor_cont_id = cmd;
//     return true;
// }

uint64_t ec::Facade::MonitorFacade::CAdvisor::getContCPULimit(const std::string agent_ip, const std::string docker_container_id) {
    std::string res;
    JSONFacade jsonFacade;
    jsonFacade.getJSONRequest("http://" + agent_ip + ":8080/api/v1.2/docker/" + docker_container_id, res);
    std::cout << res << std::endl;
}


uint64_t ec::Facade::MonitorFacade::CAdvisor::getContMemLimit(const std::string agent_ip, const std::string docker_container_id) {
    std::string tmp;
    uint64_t res;
    JSONFacade jsonFacade;
    std::string req_tmp = "http://" + agent_ip + ":8080/api/v2.0/stats/" + docker_container_id + "?type=docker";
    std::cout << req_tmp << std::endl;
    jsonFacade.getJSONRequest(req_tmp, tmp);
    res = jsonFacade.parseCAdvisorResponseLimits(tmp, "memory");
    //std::cout << res << std::endl;
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