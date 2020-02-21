#ifndef JSON_FACADE_H
#define JSON_FACADE_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <cpprest/http_client.h>
#include <cpprest/json.h> 

class JSONFacade {
    public:
        JSONFacade() {}
        void parseAppName(); 
        void parseAppImages();
        void parseIPAddresses();
        void parseGCMIPAddress();
        int parseFile(const std::string fileName);

        // Getters
        std::string getAppName() {
            return _app_name;
        }
        std::vector<std::string> getAppImages() {
            return _app_images;
        }
        std::vector<std::string> getAgentIPs() {
            return _agent_ips;
        }
        std::string getGCMIP() {
            return _gcm_ip;
        }

        std::string createJSONPodDef(const std::string app_name, const std::string app_image);
        std::string postJSONRequest(const std::string url, const std::string jsonRequest);
        std::string getJSONRequest(const std::string urlRequest);
        std::vector<std::string> getNodesFromResponse(const std::string jsonResp);
        std::string getNodeIPFromResponse(std::string jsonResp);

    private:
        web::json::value _val;
        std::string _app_name;
        std::vector<std::string> _app_images;
        std::vector<std::string> _agent_ips; 
        std::string _gcm_ip;   
};

#endif //JSON_FACADE_H
