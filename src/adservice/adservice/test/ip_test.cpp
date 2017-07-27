#include <mtty/constants.h>
#include <mtty/aerospike.h>
#include "core/core_ip_manager.h"
#include "core/core_scenario_manager.h"
#include <iostream>
#include <string>


using namespace adservice::server;

bool inDebugSession = false;
void * debugSession = nullptr;
MT::common::Aerospike aerospikeClient;

int main(int argc,char** argv){
    IpManager::init();
    IpManager& ipManager = IpManager::getInstance();
    ScenarioManagerPtr scenarioManager = ScenarioManager::getInstance();
    if(argc>=2){
        std::cout<<argv[1]<<":"<<ipManager.getAreaByIp(argv[1])<<std::endl;
        std::cout<<argv[1]<<":"<<scenarioManager->getScenario(argv[1])<<std::endl;
    }
    return 0;
}
