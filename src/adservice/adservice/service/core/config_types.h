//
// Created by guoze.lin on 16/3/3.
//

#ifndef ADCORE_CONFIG_TYPES_H
#define ADCORE_CONFIG_TYPES_H

#include <cppcms/json.h>
#include <map>
#include <mtty/constants.h>
#include <string>
#include <vector>

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
    struct Connection {
        std::string host;
        int port;
        Connection(const std::string & h, int p)
            : host(h)
            , port(p){};
    };
    std::vector<Connection> connections;
    std::string nameSpace;
};

struct GlobalConfig {
    ServerConfig serverConfig;
    LogConfig logConfig;
    ADSelectConfig adselectConfig;
    AdDataConfig addataConfig;
    AerospikeConfig aerospikeConfig;
};

#endif // ADCORE_CONFIG_TYPES_H
