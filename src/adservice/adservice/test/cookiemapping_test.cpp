#include "core/config_types.h"
#include "core/core_cm_manager.h"
#include "core/model/user.h"
#include "utility/utility.h"
#include <mtty/aerospike.h>
#include <mtty/constants.h>
#include <mtty/mtuser.h>
#include <ostream>
#include <sstream>
#include <sys/unistd.h>

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
        userMapping.addMapping(ADX_TANX, cypher::randomId(3));
        cmManager.updateUserMapping(userMapping);
        std::cout << "insert new userId:" << userIdPublic << std::endl;
        model::MtUserMapping anotherUserMapping
            = cmManager.getUserMappingByKey(userMapping.adxUidKey(ADX_TANX), userMapping.adxUids[ADX_TANX]);
        if (!anotherUserMapping.isValid()) {
            std::cout << "failed to fetch new inserted usermapping of userId " << userIdPublic << std::endl;
        } else {
            std::cout << "get usermapping successfully:" << std::endl;
            printUserMapping(anotherUserMapping);
        }
    }
}

void testQueryWhere()
{
    CookieMappingManager & cmManager = CookieMappingManager::getInstance();
    std::string key = "9e3645454d39e387f729d4e";
    model::MtUserMapping userMapping = cmManager.getUserMappingByKey(model::MtUserMapping::adxUidKey(ADX_TANX), key);
    if (userMapping.isValid()) {
        printUserMapping(userMapping);
    } else {
        std::cout << "failed to fetch userMapping by key " << key << std::endl;
    }
}

void testUpdateAsync()
{
    auto idSeq = CookieMappingManager::IdSeq();
    MT::User::UserID userId(idSeq.id(),idSeq.time());
    std::string userIdPublic = userId.text();
    model::MtUserMapping userMapping;
    userMapping.userId = userIdPublic;
    CookieMappingManager & cmManager = CookieMappingManager::getInstance();
    cmManager.updateUserMapping(userMapping);
    model::MtUserMapping readUserMapping = cmManager.getUserMappingByKey(userMapping.userIdKey(), userMapping.userId);
    if (readUserMapping.isValid()) {
        printUserMapping(readUserMapping);
    } else {
        std::cout << "failed to fetch userMapping by key:" << userMapping.userId;
    }
    std::string timeString = time::getCurrentTimeUtcString();
    if (!cmManager.updateMappingAdxUidAsync(userIdPublic, ADX_TANX, timeString + ".12341")) {
        std::cout << "failed to updateUserMappingAsync" << std::endl;
    }
    sleep(1);
    readUserMapping = cmManager.getUserMappingByKey(userMapping.userIdKey(), userMapping.userId);
    if (readUserMapping.isValid()) {
        printUserMapping(readUserMapping);
    } else {
        std::cout << "failed to fetch userMapping by key:" << userMapping.userId;
    }
}

void testUpdateAsyncPressure()
{
    MT::User::UserID userId(int16_t(rng::randomInt() & 0x0000FFFF));
    std::string userIdPublic = userId.text();
    model::MtUserMapping userMapping;
    userMapping.userId = userIdPublic;
    CookieMappingManager & cmManager = CookieMappingManager::getInstance();
    cmManager.updateUserMapping(userMapping);
    int64_t beginMs = time::getCurrentTimeStampMs();
    for (int i = 0; i < 1000000; i++) {
        std::string timeString = time::getCurrentTimeUtcString();
        if (!cmManager.updateMappingAdxUidAsync(userIdPublic, ADX_TANX, timeString + ".12341")) {
            std::cout << "failed to updateUserMappingAsync" << std::endl;
        }
    }
    int64_t endMs = time::getCurrentTimeStampMs();
    std::cout << "time collasped : " << (endMs - beginMs) << "ms" << std::endl;
}

void testIdSeq()
{
    for (int i = 0; i < 10; i++) {
        auto idSeq = CookieMappingManager::IdSeq();
        std::cout << "idSeq:" << idSeq.id() <<",time:"<<idSeq.time()<<std::endl;
    }
}

int main(int argc, char ** argv)
{
    std::vector<MT::common::ASConnection> connections{ MT::common::ASConnection("192.168.2.31", 3000) };
    aerospikeClient.setConnection(connections);
    aerospikeClient.connect();
    globalConfig.aerospikeConfig.connections = connections;
    globalConfig.aerospikeConfig.nameSpace = "test";
    // testGetAndSetCookieMapping();
    testQueryWhere();
    testUpdateAsyncPressure();
    testIdSeq();
    return 0;
}
