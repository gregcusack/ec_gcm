#include "../include/cAdvisorFacade.h"

uint64_t ec::Facade::MonitorFacade::CAdvisor::getContCPULimit(const std::string agent_ip, const std::string docker_container_id) {
    std::string res;
    ec::Facade::JSONFacade::json jsonFacade;
    jsonFacade.getJSONRequest("http://" + agent_ip + ":8080/api/v1.2/docker/" + docker_container_id, res);
    uint64_t ret;
    ret = jsonFacade.parseCAdvisorResponseLimits(res, "cpu", "throttled_periods");
    return ret;
}


uint64_t ec::Facade::MonitorFacade::CAdvisor::getContMemLimit(const std::string agent_ip, const std::string docker_container_id) {
   std::string res;
   ec::Facade::JSONFacade::json jsonFacade;
   jsonFacade.getJSONRequest("http://" + agent_ip + ":8080/api/v1.2/docker/" + docker_container_id, res);
   uint64_t ret;
   ret = jsonFacade.parseCAdvisorResponseLimits(res, "memory", "max_usage");
   return ret;
}