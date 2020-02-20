#include <iostream>
#include <fstream>
#include <cpprest/http_client.h>
#include <cpprest/json.h> 

class JSONFacade {
    public:
        JSONFacade() { _val = NULL; }
        void parseAppName(); 
        void parseAppImages();
        void parseFile(std::string fileName);
        std::string getAppName() {
            return _app_name;
        }
        std::vector<std::string> getAppImages() {
            return _app_images;
        }
    private:
        web::json::value _val;
        std::string _app_name;
        std::vector<std::string> _app_images;    
};