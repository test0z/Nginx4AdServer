#include "core/config_types.h"
#include "core/core_cm_manager.h"
#include "core/model/user.h"
#include "utility/utility.h"
#include <mtty/aerospike.h>
#include <mtty/constants.h>
#include <mtty/mtuser.h>
#include <ostream>
#include <sstream>

using namespace adservice::server;
using namespace adservice::core;
using namespace adservice::utility;

GlobalConfig globalConfig;
bool inDebugSession = false;
void * debugSession = nullptr;
MT::common::Aerospike aerospikeClient;

void printUserMapping(model::MtUserMapping & userMapping)
{
    std::stringstream ss;
    ss << "usermapping userId:" << userMapping.userId;
    ss << ",cypher userId:" << userMapping.cypherUid();
    ss << ",adxuid_tanx:" << userMapping.adxUids[ADX_TANX];
    std::cout << ss.str() << std::endl;
}

void testGetAndSetCookieMapping()
{
    MT::User::UserID userId(int16_t(rng::randomInt() & 0x0000FFFF));
    std::string userIdPublic = userId.text();
    CookieMappingManager & cmManager = CookieMappingManager::getInstance();
    model::MtUserMapping userMapping = cmManager.getUserMapping(userIdPublic);
    if (userMapping.isValid()) {
        printUserMapping(userMapping);
    } else {
        userMapping.reset();
        userMapping.userId = userIdPublic;
        userMapping.addMapping(ADX_TANX, "afasdfsadfasqwerdvjlsajoeir");
        cmManager.updateUserMapping(userMapping);
        std::cout << "insert new userId:" << userIdPublic << std::endl;
        model::MtUserMapping anotherUserMapping = cmManager.getUserMapping(userIdPublic);
        if (!anotherUserMapping.isValid()) {
            std::cout << "failed to fetch new inserted usermapping of userId " << userIdPublic << std::endl;
        } else {
            std::cout << "get usermapping successfully:" << std::endl;
            printUserMapping(anotherUserMapping);
        }
    }
}

void testUpdateAsync()
{
    MT::User::UserID userId(int16_t(rng::randomInt() & 0x0000FFFF));
    std::string userIdPublic = userId.text();
    model::MtUserMapping userMapping;
    userMapping.userId = userIdPublic;
    CookieMappingManager & cmManager = CookieMappingManager::getInstance();
    cmManager.updateUserMapping(userMapping);
    std::string timeString = time::getCurrentTimeUtcString();
    if (!cmManager.updateMappingAdxUidAsync(userIdPublic, ADX_TANX, timeString + ".12341")) {
        std::cout << "failed to updateUserMappingAsync" << std::endl;
    }
}

int main(int argc, char ** argv)
{
    std::vector<MT::common::ASConnection> connections{ MT::common::ASConnection("192.168.2.31", 3000) };
    aerospikeClient.setConnection(connections);
    aerospikeClient.connect();
    testGetAndSetCookieMapping();
    return 0;
}
