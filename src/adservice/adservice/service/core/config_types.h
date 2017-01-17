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
};

struct CookieMappingConfig {
    bool disabledCookieMapping{ false };
};

struct GlobalConfig {
    ServerConfig serverConfig;
    LogConfig logConfig;
    ADSelectConfig adselectConfig;
    AdDataConfig addataConfig;
    AerospikeConfig aerospikeConfig;
    CookieMappingConfig cmConfig;
};

#endif // ADCORE_CONFIG_TYPES_H
