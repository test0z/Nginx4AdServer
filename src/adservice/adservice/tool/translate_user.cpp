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

int main(int argc, char ** argv)
{
    if(argc>1){
        std::cout<<userDecode(argv[1])<<std::endl;
    }
    return 0;
}
