//
// Created by guoze.lin on 16/3/23.
//

#include "log.h"
#include "utility/utility.h"
#include <iostream>
#include <sstream>
#include <string>

using namespace std;
using namespace protocol::log;
using namespace adservice::utility::time;
using namespace adservice::utility::cypher;
using namespace adservice::utility::ip;
using namespace adservice::utility::url;

std::string ipv6ToString(const std::vector<int> & v)
{
    if (v.size() < 4) {
        return "";
    }
    char buf[16];
    char output[33];
    int * p = (int *)buf;
    *p++ = v[0];
    *p++ = v[1];
    *p++ = v[2];
    *p++ = v[3];
    toHexReadable((const uchar_t *)buf, 16, output);
    return std::string(output);
}

void printLogPhase(std::stringstream & ss, LogPhaseType & type)
{
    switch (type) {
    case LogPhaseType::CLICK:
        ss << "click" << endl;
        break;
    case LogPhaseType::BID:
        ss << "bid" << endl;
        break;
    case LogPhaseType::SHOW:
        ss << "show" << endl;
        break;
    default:
        break;
    }
}

void printLogAdInfo(std::stringstream & ss, protocol::log::AdInfo & adInfo)
{
    ss << "adInfo:" << endl;
    ss << "\tbannerId:" << adInfo.bannerId << endl;
    ss << "\tareaId:" << adInfo.areaId << endl;
    ss << "\tadvId:" << adInfo.advId << endl;
    ss << "\tadxId:" << adInfo.adxid << endl;
    ss << "\tcid:" << adInfo.cid << endl;
    ss << "\tcpid:" << (adInfo.cpid == 0 ? adInfo.advId : adInfo.cpid) << endl;
    ss << "\timpId:" << adInfo.imp_id << endl;
    ss << "\tmid:" << adInfo.mid << endl;
    ss << "\tpid:" << adInfo.pid << endl;
    ss << "\tadxpid:" << adInfo.adxpid << endl;
    ss << "\tsid:" << adInfo.sid << endl;
    ss << "\tlanding url:" << urlDecode(adInfo.landingUrl) << endl;
    ss << "\tclickId:" << adInfo.clickId << endl;
    ss << "\tbidPrice:" << adInfo.bidPrice << endl;
    ss << "\tofferPrice:" << adInfo.offerPrice << endl;
    ss << "\tcost:" << adInfo.cost << endl;
    ss << "\tadxuid:" << adInfo.adxuid << endl;
    ss << "\tbidSize:" << adInfo.bidSize << endl;
    ss << "\tppid:" << adInfo.ppid << endl;
    ss << "\torderId:" << adInfo.orderId << endl;
}

void printLogGeoInfo(std::stringstream & ss, protocol::log::GeoInfo & geoInfo)
{
    ss << "geoInfo:" << endl;
    ss << "\tlatitude:" << geoInfo.latitude << endl;
    ss << "\tlongitude:" << geoInfo.longitude << endl;
    ss << "\tcountry:" << geoInfo.country << endl;
    ss << "\tprovince:" << geoInfo.province << endl;
    ss << "\tcity:" << geoInfo.city << endl;
    ss << "\tdistrict:" << geoInfo.district << endl;
}

void printLogUserInfo(std::stringstream & ss, protocol::log::UserInfo & userInfo)
{
    ss << "userInfo:" << endl;
    ss << "\tage:" << userInfo.age << endl;
    ss << "\tsex:" << userInfo.sex << endl;
    ss << "\tinterest:" << userInfo.interest << endl;
}

void printLogIpInfo(std::stringstream & ss, protocol::log::IPInfo & ipInfo)
{
    ss << "ipInfo:" << endl;
    ss << "\tipv4:" << ipIntToString(ipInfo.ipv4) << endl;
    ss << "\tipv6:" << ipv6ToString(ipInfo.ipv6) << endl;
    ss << "\tproxy:" << ipInfo.proxy << endl;
}

void printLogUserId(std::stringstream & ss, const std::string & userId)
{
    CypherResult128 cypherResult;
    memcpy((void *)cypherResult.bytes, (void *)userId.c_str(), userId.length());
    DecodeResult64 decodeResult64;
    cookiesDecode(cypherResult, decodeResult64);
    ss << "userId:" << userId << ",code:" << decodeResult64.dword
       << ",first:" << timeStringFromTimeStamp(getMttyTimeBegin() + decodeResult64.words[0])
       << ",second:" << decodeResult64.words[1] << endl;
}

std::string getLogItemString(protocol::log::LogItem & log)
{
    std::stringstream ss;
    ss << "=====================================logItem=========================================" << endl;
    printLogPhase(ss, log.logType);
    printLogUserId(ss, log.userId);
    ss << "utc timestamp:" << log.timeStamp << "," << timeStringFromTimeStamp(log.timeStamp) << endl;
    ss << "userAgent:" << log.userAgent << endl;
    ss << "host:" << log.host << endl;
    ss << "path:" << log.path << endl;
    ss << "referer:" << urlDecode(log.referer) << endl;
    ss << "reqMethod:" << (log.reqMethod == 1 ? "GET" : "POST") << endl;
    ss << "reqStatus:" << log.reqStatus << endl;
    ss << "deviceInfo:" << log.deviceInfo << endl;
    ss << "jsInfo:" << log.jsInfo << endl;
    ss << "pageInfo:" << log.pageInfo << endl;
    ss << "traceId:" << log.traceId << endl;
    ss << "click coord:(" << log.clickx << "," << log.clicky << ")" << endl;
    printLogAdInfo(ss, log.adInfo);
    printLogGeoInfo(ss, log.geoInfo);
    printLogUserInfo(ss, log.userInfo);
    printLogIpInfo(ss, log.ipInfo);
    ss << "=====================================================================================" << endl;
    return ss.str();
}
