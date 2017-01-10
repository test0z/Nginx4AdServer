#include "utility/utility.h"
#include <mtty/constants.h>
#include <mtty/aerospike.h>
#include <iostream>
#include <string>


using namespace adservice::utility;

bool inDebugSession = false;
void * debugSession = nullptr;
MT::common::Aerospike aerospikeClient;

int main(int argc,char** argv){
    if(argc==2){
	cppcms::json::value doc;
	bool result = json::parseJson(argv[1],doc);
	std::cout<<(result?"true":"false")<<std::endl;
        std::cout<<"doc.isNull:"<<doc.is_null()<<std::endl;
        std::cout<<"doc.is_undefined:"<<doc.is_undefined()<<std::endl;
	std::string strJson = json::toJson(doc);
	std::cout<<"reverse parse json:"<<strJson<<std::endl;
    }
    return 0;
}
