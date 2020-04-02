#include "../include/cAdvisorFacade.h"

uint64_t ec::Facade::MonitorFacade::CAdvisor::getContCPULimit(const std::string &agent_ip, const std::string &docker_container_id) {
    std::string res;
    ec::Facade::JSONFacade::json::getJSONRequest("http://" + agent_ip + ":8080/api/v1.2/docker/" + docker_container_id, res);
    return ec::Facade::JSONFacade::json::parseCAdvisorResponseLimits(res, "cpu", "throttled_periods");
}

uint64_t ec::Facade::MonitorFacade::CAdvisor::getContCPUQuota(const std::string &agent_ip, const std::string &docker_container_id) {
    std::string res;
    ec::Facade::JSONFacade::json::getJSONRequest("http://" + agent_ip + ":8080/api/v1.2/docker/" + docker_container_id, res);
    return ec::Facade::JSONFacade::json::parseCAdvisorResponseLimits(res, "cpu", "quota");
}

uint64_t ec::Facade::MonitorFacade::CAdvisor::getContCPUShares(const std::string &agent_ip, const std::string &docker_container_id) {
    std::string res;
    ec::Facade::JSONFacade::json::getJSONRequest("http://" + agent_ip + ":8080/api/v1.2/docker/" + docker_container_id, res);
    return ec::Facade::JSONFacade::json::parseCAdvisorResponseLimits(res, "cpu", "shares");
}


uint64_t ec::Facade::MonitorFacade::CAdvisor::getContMemLimit(const std::string &agent_ip, const std::string &docker_container_id) {
   std::string res;
   ec::Facade::JSONFacade::json::getJSONRequest("http://" + agent_ip + ":8080/api/v1.2/docker/" + docker_container_id, res);
   return ec::Facade::JSONFacade::json::parseCAdvisorResponseLimits(res, "memory", "limit");

}

uint64_t ec::Facade::MonitorFacade::CAdvisor::getContMemUsage(const std::string &agent_ip, const std::string &docker_container_id) {
    std::string res;
    ec::Facade::JSONFacade::json::getJSONRequest("http://" + agent_ip + ":8080/api/v1.2/docker/" + docker_container_id, res);
    return ec::Facade::JSONFacade::json::parseCAdvisorResponseLimits(res, "memory", "max_usage");
}
