//
// Created by guoze.lin on 16/3/3.
//

#ifndef ADCORE_CONFIG_TYPES_H
#define ADCORE_CONFIG_TYPES_H

#include <map>
#include <string>
#include <vector>

#include <cppcms/json.h>

#include <mtty/aerospike.h>
#include <mtty/constants.h>
#include <unordered_map>

//全局服务配置类型
struct ServerConfig {
    int loggingLevel;
};

//日志配置类型
struct LogConfig {
    std::string kafkaBroker;
    std::string kafkaTopic;
    std::string kafkaKey;
    std::string kafkaMQMaxSize;
    int partitionCnt{ 18 };
    int localLoggerThreads;
    bool kafkaLogEnable;
};

struct ADSelectConfig {
    std::string adselectNode;
    std::map<int, int> adselectTimeout;
    bool useUnixSocket;
    std::string unixSocketFile;
};

struct AdDataConfig {
    std::string addataNamespace;
    std::string adplaceSet;
    std::string bannerSet;
};

struct AerospikeConfig {
    std::vector<MT::common::ASConnection> connections;
    std::string nameSpace;
    std::unordered_map<std::string, std::string> funcNamespacess{
        { AS_NAMESPACE_COOKIEMAPPING, "nonmem" }, { AS_NAMESPACE_CROWD, "nonmem" },
        { AS_NAMESPACE_TRACE, "nonmem" },         { AS_NAMESPACE_REQCOUNTER, "nonmem" },
        { AS_NAMESPACE_FREQ, "nondisk" },         { AS_NAMESPACE_ORDER, "inmem" },
        { AS_NAMESPACE_IDSEQ, "inmem" },          { AS_NAMESPACE_TRAFFICCONTROL, "inmem" }
    };
    int64_t getTimeoutMS{ 10 };

    const std::string & funcNamespace(const std::string & funcName)
    {
        return funcNamespacess[funcName];
    }
};

struct CookieMappingConfig {
    bool disabledCookieMapping{ false };
};

struct DBConfig {
    const std::string accessUrl{ "tcp://rm-2zek1ct2g06ergoum.mysql.rds.aliyuncs.com:3306" };
    const std::string dbName{ "mtdb" };
    const std::string userName{ "mtty_root" };
    const std::string password{ "Mtkj*8888" };
};

struct URLConfig {
    const std::string sslShowUrl{ "https://show.mtty.com/s" };
    const std::string nonSSLShowUrl{ "http://show.mtty.com/s" };
    const std::string sslClickUrl{ "https://click.mtty.com/c" };
    const std::string nonSSLClickUrl{ "http://click.mtty.com/c" };
};

struct GlobalConfig {
    ServerConfig serverConfig;
    std::unordered_map<std::string, LogConfig> logConfig;
    ADSelectConfig adselectConfig;
    AdDataConfig addataConfig;
    AerospikeConfig aerospikeConfig;
    CookieMappingConfig cmConfig;
    DBConfig dbConfig;
    URLConfig urlConfig;
};

#endif // ADCORE_CONFIG_TYPES_H
