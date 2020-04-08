#ifndef JSON_FACADE_H
#define JSON_FACADE_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <cpprest/http_client.h>
#include <cpprest/json.h>
#include <unordered_map>

#define __ERROR__ -1

namespace ec {
    namespace Facade {
        namespace JSONFacade {
            class json {
            public:
                void parseAppName(); 
                void parseAppImages();
                void parseIPAddresses();
                void parsePodNames();
                void parseGCMIPAddress();
                void parseSpecs();
                int parseFile(const std::string &fileName);

                // Getters
                const std::string &getAppName() { return _app_name;}
                const std::vector<std::string> &getAppImages() {return _app_images;}
                const std::vector<std::string> &getAgentIPs() {return _agent_ips;}
                const std::vector<std::string> &getPodNames() {return _pod_names;}
                const std::string &getGCMIP() {return _gcm_ip;}

                uint64_t get_mem();
                uint64_t get_cpu();
                uint64_t get_ports();
                uint64_t get_net();

                static void createJSONPodDef(const std::string &app_name, const std::string &app_image, const std::string &pod_name, std::string &response);
                static void postJSONRequest(const std::string &url, const std::string &jsonRequest, std::string &jsonResp);
                static void getJSONRequest(const std::string &urlRequest, std::string &jsonResp);
                static void getNodesFromResponse(const std::string &jsonResp, std::vector<std::string> &resultNodes);
                static void getNodeIPFromResponse(const std::string &jsonResp, std::string &tmp_ip);

                // Parse Container Stats
                static void getStringResponseFromURL(const std::string &urlRequest, std::string &jsonResp);
                static uint64_t parseCAdvisorResponseSpecs(const std::string &jsonResp, const std::string &resource, const std::string &type);
                static uint64_t parseCAdvisorCPUResponseStats(const std::string &jsonResp, const std::string &resource, const std::string &type);
                static uint64_t parseCAdvisorResponseStats(const std::string &jsonResp, const std::string &resource, const std::string &type);

                 // Parse Machine Stats
                static uint64_t parseCAdvisorMachineStats(const std::string &jsonResp, const std::string &type);

            private:
                web::json::value _val;
                std::string _app_name;
                std::vector<std::string> _app_images;
                std::vector<std::string> _agent_ips;
                std::vector<std::string> _pod_names;
                std::string _gcm_ip;
                static std::unordered_map<std::string, uint64_t> _specs;

                static void set_pod_limits(web::json::value &cont);
            };
        }
    }
}

#endif //JSON_FACADE_H