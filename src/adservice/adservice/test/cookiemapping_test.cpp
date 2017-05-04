#include "core/config_types.h"
#include "core/core_cm_manager.h"
#include "core/model/user.h"
#include "utility/utility.h"
#include <mtty/aerospike.h>
#include <mtty/constants.h>
#include <mtty/mtuser.h>
#include <mtty/entities.h>
#include <mtty/usershowcounter.h>
#include <iostream>
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
    ss << ",outeridkey:"<<userMapping.outerUserKey;
    ss<<",outerid:"<<userMapping.outerUserId;
    std::cout << ss.str() << std::endl;
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



void testMd5(){
    std::string output = cypher::md5_encode("helloworld");
    std::cout<<"md5 of helloworld:"<<output<<std::endl;
    output = cypher::md5_encode("CC7D4D04-C75A-41CC-8705-8C5880BD89C6");
    std::cout<<"md5 of CC7D4D04-C75A-41CC-8705-8C5880BD89C6:"<<output<<std::endl;
}


void testTranslateUser(const char* user){
    MT::User::UserID u(user,true);
    if(u.isValid()){
        std::cout<<u.text()<<std::endl;
    }else{
        std::cout<<"invalid userId"<<std::endl;
    }
}

void testGetAndSetCookieMapping()
{
    MT::User::UserID userId(int16_t(rng::randomInt() & 0x0000FFFF));
    std::string userIdPublic = userId.text();
    CookieMappingManager & cmManager = CookieMappingManager::getInstance();
    bool isTimeout = false;
    model::MtUserMapping userMapping = cmManager.getUserMappingByKey("adxuid_1","mynameistom",false,isTimeout);
    if (userMapping.isValid()) {
        printUserMapping(userMapping);
    } else {
        userMapping.reset();
        userMapping.userId = userIdPublic;
        cmManager.updateMappingAdxUid(userIdPublic,1,"mynameistom");
        std::cout << "insert new userId:" << userIdPublic << std::endl;
        model::MtUserMapping anotherUserMapping
            = cmManager.getUserMappingByKey(userMapping.adxUidKey(1), "mynameistom",false,isTimeout);
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
    auto idSeq = CookieMappingManager::IdSeq();
    MT::User::UserID userId(idSeq.id(),idSeq.time());
    std::string userIdPublic = userId.text();
    model::MtUserMapping userMapping;
    userMapping.userId = userIdPublic;
    bool isTimeout = false;
    CookieMappingManager & cmManager = CookieMappingManager::getInstance();
    cmManager.updateMappingDeviceAsync(userIdPublic,model::MtUserMapping::idfaKey(),"1094ADD26D8AD9F21E92F67F9ACC9DA1","");
    model::MtUserMapping readUserMapping = cmManager.getUserMappingByKey(model::MtUserMapping::idfaKey(), "1094ADD26D8AD9F21E92F67F9ACC9DA1",false,isTimeout);
    if (readUserMapping.isValid()) {
        printUserMapping(readUserMapping);
    } else {
        std::cout << "failed to fetch userMapping by key:" << userMapping.userId;
    }
    sleep(1);
    readUserMapping = cmManager.getUserMappingByKey(model::MtUserMapping::idfaKey(), "1094ADD26D8AD9F21E92F67F9ACC9DA1",false,isTimeout);
    if (readUserMapping.isValid()) {
        printUserMapping(readUserMapping);
    } else {
        std::cout << "failed to fetch userMapping by key:" << userMapping.userId;
    }
}

int main(int argc, char ** argv)
{
    if(argc>1){
        testTranslateUser(argv[1]);
        return 0;
    }
    std::vector<MT::common::ASConnection> connections{ MT::common::ASConnection("192.168.2.31", 3000) };
    aerospikeClient.setConnection(connections);
    aerospikeClient.connect();
    globalConfig.aerospikeConfig.connections = connections;
    globalConfig.aerospikeConfig.nameSpace = "test";
    // testGetAndSetCookieMapping();
    //testQueryWhere();
    //testUpdateAsyncPressure();
    //testIdSeq();
    //testAerospikeBatch();
    //testMd5();
    return 0;
}
