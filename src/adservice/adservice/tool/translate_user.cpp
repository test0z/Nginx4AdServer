#include "core/config_types.h"
#include "core/core_cm_manager.h"
#include "core/model/user.h"
#include "core/model/source_id.h"
#include "utility/utility.h"
#include <mtty/aerospike.h>
#include <mtty/constants.h>
#include <mtty/mtuser.h>
#include <mtty/entities.h>
#include <mtty/usershowcounter.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/unistd.h>
#include "logging.h"
#include <algorithm>

using namespace adservice::server;
using namespace adservice::core;
using namespace adservice::utility;

GlobalConfig globalConfig;
bool inDebugSession = false;
void * debugSession = nullptr;

std::string userDecode(const std::string& input){
        MT::User::UserID clientUid(input, true);
        return clientUid.text();
}

std::string md5DeviceId(const std::string& input){
    return cypher::md5_encode(stringtool::toupper(input));
}

int main(int argc, char ** argv)
{
    if(argc>1){
        std::string opt = argv[1];
        if(opt == "decode"){
            std::cout<<userDecode(argv[2])<<std::endl;
        }else if (opt == "encodedevice"){
            std::cout<<md5DeviceId(argv[2])<<std::endl;
        }
    }
    return 0;
}
