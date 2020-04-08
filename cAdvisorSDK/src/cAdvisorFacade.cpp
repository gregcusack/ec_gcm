#include "../include/cAdvisorFacade.h"

uint64_t ec::Facade::MonitorFacade::CAdvisor::getContCPUThrottledPeriods(const std::string &agent_ip, const std::string &docker_container_id) {
    std::string res;
    ec::Facade::JSONFacade::json::getJSONRequest("http://" + agent_ip + ":8080/api/v1.3/docker/" + docker_container_id, res);
    return ec::Facade::JSONFacade::json::parseCAdvisorCPUResponseStats(res, "cpu", "throttled_periods");
}

uint64_t ec::Facade::MonitorFacade::CAdvisor::getContCPUQuota(const std::string &agent_ip, const std::string &docker_container_id) {
    std::string res;
    ec::Facade::JSONFacade::json::getJSONRequest("http://" + agent_ip + ":8080/api/v1.3/docker/" + docker_container_id, res);
    return ec::Facade::JSONFacade::json::parseCAdvisorResponseSpecs(res, "cpu", "quota");
}

uint64_t ec::Facade::MonitorFacade::CAdvisor::getContCPUShares(const std::string &agent_ip, const std::string &docker_container_id) {
    std::string res;
    ec::Facade::JSONFacade::json::getJSONRequest("http://" + agent_ip + ":8080/api/v1.3/docker/" + docker_container_id, res);
    return ec::Facade::JSONFacade::json::parseCAdvisorResponseSpecs(res, "cpu", "limit");
}


uint64_t ec::Facade::MonitorFacade::CAdvisor::getContMemLimit(const std::string &agent_ip, const std::string &docker_container_id) {
   std::string res;
   ec::Facade::JSONFacade::json::getJSONRequest("http://" + agent_ip + ":8080/api/v1.3/docker/" + docker_container_id, res);
   return ec::Facade::JSONFacade::json::parseCAdvisorResponseSpecs(res, "memory", "limit");

}

uint64_t ec::Facade::MonitorFacade::CAdvisor::getContMemUsage(const std::string &agent_ip, const std::string &docker_container_id) {
    std::string res;
    ec::Facade::JSONFacade::json::getJSONRequest("http://" + agent_ip + ":8080/api/v1.3/docker/" + docker_container_id, res);
    return ec::Facade::JSONFacade::json::parseCAdvisorResponseStats(res, "memory", "max_usage");
}
