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
#include <algorithm>
#include "logging.h"

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



void testMd5(){
    std::string output = cypher::md5_encode("helloworld");
    std::cout<<"md5 of helloworld:"<<output<<std::endl;
    output = cypher::md5_encode("CC7D4D04-C75A-41CC-8705-8C5880BD89C6");
    std::cout<<"md5 of CC7D4D04-C75A-41CC-8705-8C5880BD89C6:"<<output<<std::endl;
}

std::string getUidByDeviceId(const std::string& setName,const std::string& pk)
{
    std::string md5deviceId = cypher::md5_encode(pk);
    CookieMappingManager & cmManager = CookieMappingManager::getInstance();
    bool isTimeout;
    model::MtUserMapping userMapping = cmManager.getUserMappingByKey(setName,md5deviceId,true,isTimeout);
    if (userMapping.isValid()) {
        return userMapping.userId;
    }else{
        return "";
    }
}

void grabUserSourceRecord(const std::string& uid,int advId){
    if(uid.empty()){
        return;
    }
    std::string sourceId;
    model::SourceId sourceIdEntity;
    MT::common::ASKey key(
        globalConfig.aerospikeConfig.nameSpace.c_str(), "source_id_index", (uid + std::to_string(advId)).c_str());
    try {
        aerospikeClient.get(key, sourceIdEntity);
        sourceId = sourceIdEntity.get();
    } catch (MT::common::AerospikeExcption & e) {
        LOG_ERROR << "获取source_id失败！userid:" << uid;
        return;
    }

    model::SourceRecord sourceRecord;
    if (!sourceId.empty()) {
        MT::common::ASKey key(globalConfig.aerospikeConfig.nameSpace.c_str(), "source_id", sourceId.c_str());
        try {
            aerospikeClient.get(key, sourceRecord);
        } catch (MT::common::AerospikeExcption & e) {
            LOG_ERROR << "获取source record失败！sourceId:" << sourceId;
            return ;
        }
    }
    std::cout<<"adv:"<<sourceRecord.advId()<<",adx:"<<sourceRecord.adxId()<<",pid:"<<sourceRecord.pid()<<",sid:"<<sourceRecord.sid()
            <<",cid:"<<sourceRecord.createId()<<std::endl;
}

int main(int argc, char ** argv)
{
    if(argc<3){
        std::cerr<<"usage: "<<argv[0]<<" [file] [advid]"<<std::endl;
        return 0;
    }
    int advId = std::stoi(std::string(argv[2]));
    std::vector<MT::common::ASConnection> connections{ MT::common::ASConnection("192.168.2.112", 3000) };
    aerospikeClient.setConnection(connections);
    aerospikeClient.connect();
    globalConfig.aerospikeConfig.connections = connections;
    globalConfig.aerospikeConfig.nameSpace = "cm";
    AdServiceLog::globalLoggingLevel = LoggingLevel::INFO_;
    try{
        std::fstream fs(argv[1],std::ios_base::in);
        std::map<std::string,std::string> imeiUids;
        std::map<std::string,std::string> idfaUids;
        std::map<std::string,std::string> macUids;
        std::map<std::string,std::string> androidUids;
        if(fs.good()){
            while(!fs.eof()){
                char buf[256];
                fs.getline(buf,sizeof(buf));
                if(fs.eof()){
                    break;
                }
                std::string str(buf);
                if(str.length()<=15){//imei
                    imeiUids.insert({str,""});
                }else if(str.find("-")!=std::string::npos){//idfa
                    std::transform(str.begin(),str.end(),str.begin(),[](unsigned char c) { return std::toupper(c); });
                    idfaUids.insert({str,""});
                }else if(str.find(":")!=std::string::npos){//mac
                    macUids.insert({str,""});
                }else{ //android id
                    androidUids.insert({str,""});
                }
            }
            fs.close();
        }
        std::cout<<"imei users:"<<std::endl;
        for(auto& iter : imeiUids){
            iter.second = getUidByDeviceId(model::MtUserMapping::imeiKey(),iter.first);
            std::cout<<iter.first<<" => "<<iter.second<<std::endl;
            grabUserSourceRecord(iter.second,advId);
        }
        std::cout<<"idfa users:"<<std::endl;
        for(auto iter : idfaUids){
            iter.second = getUidByDeviceId(model::MtUserMapping::idfaKey(),iter.first);
            std::cout<<iter.first<<" => "<<iter.second<<std::endl;
        }
        std::cout<<"mac users:"<<std::endl;
        for(auto iter : macUids){
            iter.second = getUidByDeviceId(model::MtUserMapping::macKey(),iter.first);
            std::cout<<iter.first<<" => "<<iter.second<<std::endl;
        }
        std::cout<<"androidid users:"<<std::endl;
        for(auto iter : androidUids){
            iter.second = getUidByDeviceId(model::MtUserMapping::androidIdKey(),iter.first);
            std::cout<<iter.first<<" => "<<iter.second<<std::endl;
        }
    }catch(std::exception& e){
        std::cerr<<"exception:"<<e.what();
    }

    return 0;
}
