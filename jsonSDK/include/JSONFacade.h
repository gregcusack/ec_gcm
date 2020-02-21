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

        std::string createK8PodDef(const std::string app_name, const std::string app_image);

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


    private:
        web::json::value _val;
        std::string _app_name;
        std::vector<std::string> _app_images;
        std::vector<std::string> _agent_ips; 
        std::string _gcm_ip;   
};