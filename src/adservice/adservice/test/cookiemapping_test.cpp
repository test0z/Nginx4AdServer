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

void testUpdateSync()
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
    if (!cmManager.updateMappingAdxUid(userIdPublic, ADX_TANX, timeString + ".12341")) {
        std::cout << "failed to updateUserMappingAsync" << std::endl;
    }
    readUserMapping = cmManager.getUserMappingByKey(userMapping.userIdKey(), userMapping.userId);
    if (readUserMapping.isValid()) {
        printUserMapping(readUserMapping);
    } else {
        std::cout << "failed to fetch userMapping by key:" << userMapping.userId;
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

void testUserId(){
    auto idSeq = CookieMappingManager::IdSeq();
    MT::User::UserID userId(idSeq.id(),idSeq.time());
    std::string cypherId = userId.cipher();
    std::string userPublicId = userId.text();
    std::cout<<"origin userId,cypher:"<<cypherId<<",public:"<<userPublicId<<std::endl;
    MT::User::UserID userId2(cypherId,true);
    std::cout<<"from cypher userId,cypher:"<<userId2.cipher()<<",public:"<<userId2.text()<<std::endl;
    MT::User::UserID userId3(userPublicId,false);
    std::cout<<"from public userId,cypher:"<<userId3.cipher()<<",public:"<<userId3.text()<<std::endl;
}

struct asBatchCBData {
    std::vector<int64_t> * data1;
    std::vector<std::string> * data2;
};

bool asBatchCB(const as_batch_read * result, uint32_t n, void * udata)
{
    asBatchCBData * cbData = (asBatchCBData *)udata;
    std::vector<int64_t> & orderCounts = *(cbData->data1);
    std::vector<std::string> & binNames = *(cbData->data2);
    for (int i = 0; i < n; i++) {
        if (result[i].result == AEROSPIKE_OK) {
            const as_record & record = result[i].record;
            int amount = as_record_get_int64(&record, binNames[i].c_str(), 0);
            std::cout<<"amount:"<<amount<<std::endl;
            orderCounts.push_back(amount);
        } else {
            std::cout<<"amount:"<<0<<std::endl;
            orderCounts.push_back(0);
        }
    }
    return true;
}

void testAerospikeBatch(){
    std::vector<std::string> freqKeys;
    std::string mtUserId="2016121201551054800000";
    std::vector<int64_t> sIds = {100678,100755,100799};
    for (auto & sId : sIds) {
        freqKeys.push_back(mtUserId + ":" + std::to_string(sId));
    }
    if (freqKeys.size() > 0) {
        MT::common::ASBatch asBatch("test", "user-freq", freqKeys);
        std::vector<std::string> binNames(freqKeys.size(), "s");
        std::vector<int64_t> freqCounts;
        asBatchCBData cbData;
        cbData.data1 = &freqCounts;
        cbData.data2 = &binNames;
        aerospikeClient.execBatchRead(asBatch, &asBatchCB, &cbData);
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
    testAerospikeBatch();
    return 0;
}
