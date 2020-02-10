#include <iostream>
#include <fstream>

#include "GlobalCloudManager.h"
#include "ElasticContainer.h"
#include "SubContainer.h"
#include "types/msg.h"
#include <cpprest/http_client.h>
#include <cpprest/json.h> // JSON library

using namespace web;                        // Common features like URIs, JSON.

#define GCM_PORT        8888             //Not sure if we need a port here tbh

int main(int argc, char* argv[]){

    // Check the number of parameters
    if (argc < 2) {
        // Tell the user how to run the program
        std::cerr << "Usage: " << argv[0] << " <path-to-json-file>" << std::endl;
        return 1;
    }
    std::string jsonFile = argv[1];
    std::vector<std::string>    agent_ips{"192.168.6.8"};//), "127.0.0.1", "127.0.0.1"};
    std::vector<uint16_t>       server_ports{4444};

    auto *gcm = new ec::GlobalCloudManager("192.168.6.10", GCM_PORT, agent_ips, server_ports);

    /* Here, we'll parse the input JSON file and then pass the specifics 
       into the server object
        Steps in parsing the JSON app definition
        Step 1: Parase JSON app definition file for application limits, images, etc
        Step 2: Create a server for each agent with the pod name (same as the container name)
                and each pod limit (this needs to be further discussed, right now, we just give it a minimum resource limit)
                i.e. if we have two agents running and 2 images, we should create 4 pods in total, right? (confirm this)
       Todo: change the server-first architecture to the manager-first architecture
    */
    std::cout<<"[dbg] this is the json file: " <<jsonFile << std::endl;
    json::value val;                                          // JSON read from input file
    try {
        utility::ifstream_t      file_stream(jsonFile);                                // filestream of working file
        utility::stringstream_t  stream;                                          // string stream for holding JSON read from file
        if (file_stream) {                                                    
            stream << file_stream.rdbuf();                                         // stream results of reading from file stream into string stream
            file_stream.close();                                              // close the filestream
        }
        val = json::value::parse(stream);                               // parse the resultant string stream.
    }
    catch (web::json::json_exception excep) {
        std::cout << "ERROR Parsing JSON file: ";
        std::cout << excep.what();
        return 1;
    }

    auto app_name = val.at(U("name")).as_string();
    auto app_images = val.at(U("images")).as_array();
    std::vector<std::string> app_images_strings;
    for (int i = 0; i < app_images.size(); i++){
        app_images_strings.push_back(app_images[i].as_string());
        //std::cout << app_images_strings[i] << std::endl;
    }

    // Todo: implement this later.. the app specs are all just placeholders right now
    // auto app_specs = val.at(U("specs")).as_array();
    // for (int i = 0; i<app_specs.size(); ++i)
    // {
    //     auto mem_spec = app_specs[i].at(U("mem")).as_string();
    //     auto cpu_spec = app_specs[i].at(U("cpu")).as_string();
    //     auto port_spec = app_specs[i].at(U("ports")).as_string();
    //     auto net_spec = app_specs[i].at(U("net")).as_string();
        
    //     std::cout << mem_spec << std::endl;
    //     std::cout << cpu_spec << std::endl;
    //     std::cout << port_spec << std::endl;
    //     std::cout << net_spec << std::endl;

    // }
    
    for(const auto &i : server_ports) {
        gcm->create_server();
    }
    std::cout << "num servers: " << gcm->get_servers().size() << std::endl;

    gcm->run(app_name, app_images_strings);

    delete gcm;


    return 0;
}
