#include "../include/JSONFacade.h"

void JSONFacade::parseAppName() {
    if(_val == NULL) {
        return;
    }
   _app_name = _val.at(U("name")).as_string();
}

void JSONFacade::parseAppImages() {
    if(_val == NULL) {
        return;
    }
    auto app_images = _val.at(U("images")).as_array();
    for(const auto &i : app_images) {
        _app_images.push_back(i.as_string());
    }
}

void JSONFacade::parseFile(std::string fileName) {
    try {
        utility::ifstream_t      file_stream(fileName);                                
        utility::stringstream_t  stream;                                         
        if (file_stream) {                                                    
            stream << file_stream.rdbuf();                                        
            file_stream.close();                                            
        }
        _val = web::json::value::parse(stream);
        parseAppName();
        parseAppImages();                              
    }
    catch (web::json::json_exception excep) {
        std::cout << "ERROR Parsing JSON file: ";
        return;
    }
}
