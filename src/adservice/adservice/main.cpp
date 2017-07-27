#include <mutex>
#include <ostream>
#include <string>

#include <sys/types.h>
#include <unistd.h>

#include <boost/algorithm/string.hpp>

#include <mtty/constants.h>
#include <mtty/requestcounter.h>
#include <mtty/trafficcontrollproxy.h>

extern "C" {
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_hash.h>
#include <ngx_http.h>
#include <ngx_http_request.h>
#include <ngx_string.h>
}

#include "core/adselectv2/ad_select_client.h"
#include "core/config_types.h"
#include "core/core_ad_sizemap.h"
#include "core/core_ip_manager.h"
#include "core/core_scenario_manager.h"
#include "core/core_typetable.h"
#include "core/logic/bid_query_task.h"
#include "core/logic/click_query_task.h"
#include "core/logic/mapping_query_task.h"
#include "core/logic/show_query_task.h"
#include "core/logic/tool_query_task.h"
#include "core/logic/trace_task.h"
#include "core/logpusher/log_pusher.h"
#include "logging.h"
#include "protocol/debug/debug.pb.h"
#include "protocol/guangyin/guangyin_bidding_handler.h"
#include "utility/utility.h"

struct LocationConf {
    //运行日志级别
    ngx_uint_t logginglevel;
    // Kafka连接配置
    // kafka节点
    ngx_str_t kafkabroker;
    // kafkatopic
    ngx_str_t kafkatopic;
    // kafkakey
    ngx_str_t kafkakey;
    // kafka传输队列大小
    ngx_str_t kafkamqmaxsize;
    //是否开启kafka日志
    ngx_uint_t kafkaloggerenable;
    // kafka 分区数
    ngx_uint_t kafkapartionscnt;
    // Kafka连接配置
    // bid kafka节点
    ngx_str_t bidkafkabroker;
    // bid kafkatopic
    ngx_str_t bidkafkatopic;
    // bid kafkakey
    ngx_str_t bidkafkakey;
    // bid kafka传输队列大小
    ngx_str_t bidkafkamqmaxsize;
    //是否开启bid kafka日志
    ngx_uint_t bidkafkaloggerenable;
    // bid kafka 分区数
    ngx_uint_t bidkafkapartionscnt;
    //如果不开启kafka日志,本地业务日志的线程数
    ngx_uint_t localloggerthreads;
    // adselect配置
    // adselect 服务url
    ngx_str_t adselectentry;
    // adselect 服务超时时间
    ngx_str_t adselecttimeout;
    // aerospike config
    // aerospike 节点
    ngx_str_t asnode;
    // aerospike 默认namespace
    ngx_str_t asnamespace;
    // aerospike ns cookiemapping
    ngx_str_t as_namespace_cookiemapping;
    // aerospike ns crowd
    ngx_str_t as_namespace_crowd;
    // aerospike ns freq
    ngx_str_t as_namespace_freq;
    // aerospike ns idseq
    ngx_str_t as_namespace_idseq;
    // aerospike ns order
    ngx_str_t as_namespace_order;
    // aerospike ns trace
    ngx_str_t as_namespace_trace;
    // aerospike ns trafficcontroll
    ngx_str_t as_namespace_trafficcontroll;
    // aerospike get api timeout ms
    ngx_uint_t as_get_timeout_ms;
    // working directory
    ngx_str_t workdir;
    // disable cookiemapping
    ngx_str_t disabledCookieMapping;
    //数据库链接
    ngx_str_t db_access_url;
    //库名
    ngx_str_t db_scheme_name;
    //用户
    ngx_str_t db_user;
    //密码
    ngx_str_t db_password;

    // ssl 曝光url
    ngx_str_t ssl_show_base_url;
    //非ssl 曝光url
    ngx_str_t show_base_url;
    // ssl 点击url
    ngx_str_t ssl_click_base_url;
    //非ssl 点击url
    ngx_str_t click_base_url;
};

static ngx_int_t adservice_init(ngx_conf_t * cf);
static ngx_int_t adservice_handler(ngx_http_request_t * r);

char * parseConfStr(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    char * rv = ngx_conf_set_str_slot(cf, cmd, conf);
    return rv;
}

char * parseConfNum(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    char * rv = ngx_conf_set_num_slot(cf, cmd, conf);
    return rv;
}

#define COMMAND_ITEM(name, configstruct, variable, parser)                                      \
    {                                                                                           \
        ngx_string(name), NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1, parser, NGX_HTTP_LOC_CONF_OFFSET, \
            offsetof(configstruct, variable), 0                                                 \
    }

static ngx_command_t commands[]
    = { COMMAND_ITEM("logging_level", LocationConf, logginglevel, parseConfNum),
        COMMAND_ITEM("kafka_broker", LocationConf, kafkabroker, parseConfStr),
        COMMAND_ITEM("kafka_topic", LocationConf, kafkatopic, parseConfStr),
        COMMAND_ITEM("kafka_key", LocationConf, kafkakey, parseConfStr),
        COMMAND_ITEM("kafka_mqmaxsize", LocationConf, kafkamqmaxsize, parseConfStr),
        COMMAND_ITEM("kafka_logger_enable", LocationConf, kafkaloggerenable, parseConfNum),
        COMMAND_ITEM("kafka_partition_cnt", LocationConf, kafkapartionscnt, parseConfNum),
        COMMAND_ITEM("bid_kafka_broker", LocationConf, bidkafkabroker, parseConfStr),
        COMMAND_ITEM("bid_kafka_topic", LocationConf, bidkafkatopic, parseConfStr),
        COMMAND_ITEM("bid_kafka_key", LocationConf, bidkafkakey, parseConfStr),
        COMMAND_ITEM("bid_kafka_mqmaxsize", LocationConf, bidkafkamqmaxsize, parseConfStr),
        COMMAND_ITEM("bid_kafka_logger_enable", LocationConf, bidkafkaloggerenable, parseConfNum),
        COMMAND_ITEM("bid_kafka_partition_cnt", LocationConf, bidkafkapartionscnt, parseConfNum),
        COMMAND_ITEM("local_logger_thread", LocationConf, localloggerthreads, parseConfNum),
        COMMAND_ITEM("adselect_entry", LocationConf, adselectentry, parseConfStr),
        COMMAND_ITEM("adselect_timeout", LocationConf, adselecttimeout, parseConfStr),
        COMMAND_ITEM("as_node", LocationConf, asnode, parseConfStr),
        COMMAND_ITEM("as_namespace", LocationConf, asnamespace, parseConfStr),
        COMMAND_ITEM("as_namespace_cookiemapping", LocationConf, as_namespace_cookiemapping, parseConfStr),
        COMMAND_ITEM("as_namespace_crowd", LocationConf, as_namespace_crowd, parseConfStr),
        COMMAND_ITEM("as_namespace_freq", LocationConf, as_namespace_freq, parseConfStr),
        COMMAND_ITEM("as_namespace_idseq", LocationConf, as_namespace_idseq, parseConfStr),
        COMMAND_ITEM("as_namespace_order", LocationConf, as_namespace_order, parseConfStr),
        COMMAND_ITEM("as_namespace_trace", LocationConf, as_namespace_trace, parseConfStr),
        COMMAND_ITEM("as_namespace_trafficcontroll", LocationConf, as_namespace_trafficcontroll, parseConfStr),
        COMMAND_ITEM("as_get_timeout_ms", LocationConf, as_get_timeout_ms, parseConfNum),
        COMMAND_ITEM("workdir", LocationConf, workdir, parseConfStr),
        COMMAND_ITEM("disable_cm", LocationConf, disabledCookieMapping, parseConfStr),
        COMMAND_ITEM("db_access_url", LocationConf, db_access_url, parseConfStr),
        COMMAND_ITEM("db_scheme_name", LocationConf, db_scheme_name, parseConfStr),
        COMMAND_ITEM("db_user", LocationConf, db_user, parseConfStr),
        COMMAND_ITEM("db_password", LocationConf, db_password, parseConfStr),
        COMMAND_ITEM("ssl_show_base_url", LocationConf, ssl_show_base_url, parseConfStr),
        COMMAND_ITEM("show_base_url", LocationConf, show_base_url, parseConfStr),
        COMMAND_ITEM("ssl_click_base_url", LocationConf, ssl_click_base_url, parseConfStr),
        COMMAND_ITEM("click_base_url", LocationConf, click_base_url, parseConfStr),
        ngx_null_command };

#define INIT_NGX_STR(str, initvalue)                                 \
    {                                                                \
        str.data = (u_char *)initvalue, str.len = strlen(initvalue); \
    }
#define TRUE 1
#define FALSE 0

static void * createLocationConf(ngx_conf_t * cf)
{
    LocationConf * conf = (LocationConf *)ngx_pcalloc(cf->pool, sizeof(LocationConf));
    if (conf == nullptr) {
        return NGX_CONF_ERROR;
    }
    conf->logginglevel = NGX_CONF_UNSET_UINT;
    ngx_str_null(&conf->kafkabroker);
    ngx_str_null(&conf->kafkatopic);
    ngx_str_null(&conf->kafkakey);
    ngx_str_null(&conf->kafkamqmaxsize);
    conf->kafkaloggerenable = NGX_CONF_UNSET_UINT;
    conf->kafkapartionscnt = NGX_CONF_UNSET_UINT;
    ngx_str_null(&conf->bidkafkabroker);
    ngx_str_null(&conf->bidkafkatopic);
    ngx_str_null(&conf->bidkafkakey);
    ngx_str_null(&conf->bidkafkamqmaxsize);
    conf->bidkafkaloggerenable = NGX_CONF_UNSET_UINT;
    conf->bidkafkapartionscnt = NGX_CONF_UNSET_UINT;
    conf->localloggerthreads = NGX_CONF_UNSET_UINT;
    ngx_str_null(&conf->adselectentry);
    ngx_str_null(&conf->adselecttimeout);
    ngx_str_null(&conf->asnode);
    ngx_str_null(&conf->asnamespace);
    ngx_str_null(&conf->as_namespace_cookiemapping);
    ngx_str_null(&conf->as_namespace_crowd);
    ngx_str_null(&conf->as_namespace_freq);
    ngx_str_null(&conf->as_namespace_idseq);
    ngx_str_null(&conf->as_namespace_order);
    ngx_str_null(&conf->as_namespace_trace);
    ngx_str_null(&conf->as_namespace_trafficcontroll);
    conf->as_get_timeout_ms = NGX_CONF_UNSET_UINT;
    ngx_str_null(&conf->workdir);
    ngx_str_null(&conf->disabledCookieMapping);
    ngx_str_null(&conf->db_access_url);
    ngx_str_null(&conf->db_scheme_name);
    ngx_str_null(&conf->db_user);
    ngx_str_null(&conf->db_password);
    ngx_str_null(&conf->ssl_show_base_url);
    ngx_str_null(&conf->ssl_click_base_url);
    ngx_str_null(&conf->show_base_url);
    ngx_str_null(&conf->click_base_url);
    return conf;
}

static char * mergeLocationConf(ngx_conf_t * cf, void * parent, void * child)
{
    LocationConf * prev = (LocationConf *)parent;
    LocationConf * conf = (LocationConf *)child;

    ngx_conf_merge_uint_value(conf->logginglevel, prev->logginglevel, 3);
    ngx_conf_merge_str_value(conf->kafkabroker, prev->kafkabroker, "");
    ngx_conf_merge_str_value(conf->kafkatopic, prev->kafkatopic, "mt-log");
    ngx_conf_merge_str_value(conf->kafkakey, prev->kafkakey, "");
    ngx_conf_merge_str_value(conf->kafkamqmaxsize, prev->kafkamqmaxsize, "10000");
    ngx_conf_merge_uint_value(conf->kafkaloggerenable, prev->kafkaloggerenable, TRUE);
    ngx_conf_merge_uint_value(conf->kafkapartionscnt, prev->kafkapartionscnt, 18);
    ngx_conf_merge_str_value(conf->bidkafkabroker, prev->bidkafkabroker, "");
    ngx_conf_merge_str_value(conf->bidkafkatopic, prev->bidkafkatopic, "mt-log");
    ngx_conf_merge_str_value(conf->bidkafkakey, prev->bidkafkakey, "");
    ngx_conf_merge_str_value(conf->bidkafkamqmaxsize, prev->bidkafkamqmaxsize, "10000");
    ngx_conf_merge_uint_value(conf->bidkafkaloggerenable, prev->bidkafkaloggerenable, TRUE);
    ngx_conf_merge_uint_value(conf->bidkafkapartionscnt, prev->bidkafkapartionscnt, 18);
    ngx_conf_merge_uint_value(conf->localloggerthreads, prev->localloggerthreads, 3);
    ngx_conf_merge_str_value(conf->adselectentry, prev->adselectentry, "");
    ngx_conf_merge_str_value(conf->adselecttimeout, prev->adselecttimeout, "0:15|21:-1|98:-1|99:-1");
    ngx_conf_merge_str_value(conf->asnode, prev->asnode, "");
    ngx_conf_merge_str_value(conf->asnamespace, prev->asnamespace, "mtty");
    ngx_conf_merge_str_value(conf->as_namespace_cookiemapping, prev->as_namespace_cookiemapping, "nonmem");
    ngx_conf_merge_str_value(conf->as_namespace_crowd, prev->as_namespace_crowd, "nonmem");
    ngx_conf_merge_str_value(conf->as_namespace_freq, prev->as_namespace_freq, "nondisk");
    ngx_conf_merge_str_value(conf->as_namespace_idseq, prev->as_namespace_freq, "inmem");
    ngx_conf_merge_str_value(conf->as_namespace_order, prev->as_namespace_order, "inmem");
    ngx_conf_merge_str_value(conf->as_namespace_trace, prev->as_namespace_trace, "nonmem");
    ngx_conf_merge_str_value(conf->as_namespace_trafficcontroll, prev->as_namespace_trafficcontroll, "inmem");
    ngx_conf_merge_uint_value(conf->as_get_timeout_ms, prev->as_get_timeout_ms, 10);
    ngx_conf_merge_str_value(conf->workdir, prev->workdir, "/usr/local/nginx/sbin/");
    ngx_conf_merge_str_value(conf->disabledCookieMapping, prev->disabledCookieMapping, "no");
    ngx_conf_merge_str_value(conf->db_access_url, prev->db_access_url,
                             "tcp://rm-2zek1ct2g06ergoum.mysql.rds.aliyuncs.com:3306");
    ngx_conf_merge_str_value(conf->db_scheme_name, prev->db_scheme_name, "mtdb");
    ngx_conf_merge_str_value(conf->db_user, prev->db_user, "mtty_root");
    ngx_conf_merge_str_value(conf->db_password, prev->db_password, "Mtkj*8888");
    ngx_conf_merge_str_value(conf->ssl_show_base_url, prev->ssl_show_base_url, "https://show.mtty.com/s");
    ngx_conf_merge_str_value(conf->show_base_url, prev->show_base_url, "http://show.mtty.com/s");
    ngx_conf_merge_str_value(conf->ssl_click_base_url, prev->ssl_click_base_url, "https://click.mtty.com/c");
    ngx_conf_merge_str_value(conf->click_base_url, prev->click_base_url, "http://click.mtty.com/c");
    return NGX_CONF_OK;
}

static ngx_http_module_t moduleContext
    = { nullptr, adservice_init, nullptr, nullptr, nullptr, nullptr, createLocationConf, mergeLocationConf };

ngx_module_t modAdservice
    = { NGX_MODULE_V1, &moduleContext, commands, NGX_HTTP_MODULE, nullptr, nullptr,
        nullptr,       nullptr,        nullptr,  nullptr,         nullptr, NGX_MODULE_V1_PADDING };

ngx_int_t adservice_init(ngx_conf_t * cf)
{
    ngx_http_handler_pt * h;
    ngx_http_core_main_conf_t * cmcf;

    cmcf = (ngx_http_core_main_conf_t *)ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = (ngx_http_handler_pt *)ngx_array_push(&cmcf->phases[NGX_HTTP_CONTENT_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }
    *h = adservice_handler;
    return NGX_OK;
}

std::string parseStr(ngx_str_t * t)
{
    if (t != nullptr && t->data != nullptr && t->len > 0) {
        return std::string((char *)t->data, t->len);
    }
    return std::string();
}

std::string parseStr(ngx_str_t & t)
{
    return parseStr(&t);
}
std::string parseKey(ngx_table_elt_t & t)
{
    return parseStr(t.key);
}
std::string parseKey(ngx_table_elt_t * t)
{
    return parseStr(t->key);
}
std::string parseValue(ngx_table_elt_t & t)
{
    return parseStr(t.value);
}
std::string parseValue(ngx_table_elt_t * t)
{
    return parseStr(t->value);
}

static bool serviceInitialized = false;
static std::mutex globalMutex;
GlobalConfig globalConfig;
adservice::log::LogPusherPtr serviceLogger = nullptr;
adservice::log::LogPusherPtr bidServiceLogger = nullptr;
AdServiceLogPtr serviceLogPtr = nullptr;
adservice::adselectv2::AdSelectClientPtr adSelectClient;
ngx_log_t * globalLog;
bool inDebugSession = false;
void * debugSession = nullptr;
MT::common::Aerospike aerospikeClient;
MT::common::RequestCounter requestCounter;
int64_t uniqueIdSeq = 0;

#define NGX_STR_2_STD_STR(str) std::string((const char *)str.data, (const char *)str.data + str.len)
#define NGX_BOOL(b) (b == TRUE)

void parseConfigAeroSpikeNode(const std::string & asNode, AerospikeConfig & config)
{
    size_t prevPos = 0, nextPos;
    while ((nextPos = asNode.find(",", prevPos)) != std::string::npos) {
        size_t portPos = 0;
        if ((portPos = asNode.find(":", prevPos)) != std::string::npos) {
            config.connections.push_back(MT::common::ASConnection(
                std::string(asNode.data() + prevPos, asNode.data() + portPos), std::stoi(asNode.data() + portPos + 1)));
        } else {
            config.connections.push_back(
                MT::common::ASConnection(std::string(asNode.data() + prevPos, asNode.data() + nextPos), 3000));
        }
        prevPos = nextPos + 1;
    }
}

void parseConfigAdselectTimeout(const std::string & timeoutStr, std::map<int, int> & timeoutMap)
{
    std::vector<std::string> results;
    boost::algorithm::split(results, timeoutStr, boost::algorithm::is_any_of("|:"));
    int len = (int)results.size();
    for (int i = 0; i < len; i += 2) {
        int adx = std::stoi(results[i]);
        int timeout = std::stoi(results[i + 1]);
        timeoutMap.insert(std::make_pair(adx, timeout));
    }
}

void setGlobalLoggingLevel(int loggingLevel)
{
    AdServiceLog::globalLoggingLevel = (LoggingLevel)loggingLevel;
}

static void global_init(LocationConf * conf)
{
    std::unique_lock<std::mutex> lock(globalMutex);

    if (serviceInitialized) {
        return;
    }

    globalConfig.serverConfig.loggingLevel = (int)conf->logginglevel;
    setGlobalLoggingLevel(globalConfig.serverConfig.loggingLevel);
    LogConfig logConfig;
    logConfig.kafkaBroker = NGX_STR_2_STD_STR(conf->kafkabroker);
    logConfig.kafkaKey = NGX_STR_2_STD_STR(conf->kafkakey);
    logConfig.kafkaTopic = NGX_STR_2_STD_STR(conf->kafkatopic);
    logConfig.kafkaMQMaxSize = NGX_STR_2_STD_STR(conf->kafkamqmaxsize);
    logConfig.kafkaLogEnable = NGX_BOOL(conf->kafkaloggerenable);
    logConfig.localLoggerThreads = conf->localloggerthreads;
    logConfig.partitionCnt = conf->kafkapartionscnt;
    globalConfig.logConfig.insert({ CONFIG_LOG, logConfig });
    LogConfig bidLogConfig;
    bidLogConfig.kafkaBroker = NGX_STR_2_STD_STR(conf->bidkafkabroker);
    bidLogConfig.kafkaKey = NGX_STR_2_STD_STR(conf->bidkafkakey);
    bidLogConfig.kafkaTopic = NGX_STR_2_STD_STR(conf->bidkafkatopic);
    bidLogConfig.kafkaMQMaxSize = NGX_STR_2_STD_STR(conf->bidkafkamqmaxsize);
    bidLogConfig.kafkaLogEnable = NGX_BOOL(conf->bidkafkaloggerenable);
    bidLogConfig.localLoggerThreads = conf->localloggerthreads;
    bidLogConfig.partitionCnt = conf->bidkafkapartionscnt;
    std::string bidConfigKey = std::string("BID_") + CONFIG_LOG;
    globalConfig.logConfig.insert({ bidConfigKey, bidLogConfig });
    globalConfig.adselectConfig.adselectNode = NGX_STR_2_STD_STR(conf->adselectentry);
    std::string disableCM = NGX_STR_2_STD_STR(conf->disabledCookieMapping);
    globalConfig.cmConfig.disabledCookieMapping = disableCM == "yes";
    parseConfigAdselectTimeout(NGX_STR_2_STD_STR(conf->adselecttimeout), globalConfig.adselectConfig.adselectTimeout);

    globalConfig.aerospikeConfig.nameSpace = NGX_STR_2_STD_STR(conf->asnamespace);
    globalConfig.aerospikeConfig.funcNamespacess[AS_NAMESPACE_COOKIEMAPPING]
        = NGX_STR_2_STD_STR(conf->as_namespace_cookiemapping);
    globalConfig.aerospikeConfig.funcNamespacess[AS_NAMESPACE_CROWD] = NGX_STR_2_STD_STR(conf->as_namespace_crowd);
    globalConfig.aerospikeConfig.funcNamespacess[AS_NAMESPACE_FREQ] = NGX_STR_2_STD_STR(conf->as_namespace_freq);
    globalConfig.aerospikeConfig.funcNamespacess[AS_NAMESPACE_IDSEQ] = NGX_STR_2_STD_STR(conf->as_namespace_idseq);
    globalConfig.aerospikeConfig.funcNamespacess[AS_NAMESPACE_ORDER] = NGX_STR_2_STD_STR(conf->as_namespace_order);
    globalConfig.aerospikeConfig.funcNamespacess[AS_NAMESPACE_TRACE] = NGX_STR_2_STD_STR(conf->as_namespace_trace);
    globalConfig.aerospikeConfig.funcNamespacess[AS_NAMESPACE_TRAFFICCONTROL]
        = NGX_STR_2_STD_STR(conf->as_namespace_trafficcontroll);
    globalConfig.aerospikeConfig.getTimeoutMS = conf->as_get_timeout_ms;
    std::string asNode = NGX_STR_2_STD_STR(conf->asnode);
    parseConfigAeroSpikeNode(asNode, globalConfig.aerospikeConfig);
    aerospikeClient.setConnection(globalConfig.aerospikeConfig.connections);
    aerospikeClient.connect();

    globalConfig.dbConfig.accessUrl = NGX_STR_2_STD_STR(conf->db_access_url);
    globalConfig.dbConfig.dbName = NGX_STR_2_STD_STR(conf->db_scheme_name);
    globalConfig.dbConfig.userName = NGX_STR_2_STD_STR(conf->db_user);
    globalConfig.dbConfig.password = NGX_STR_2_STD_STR(conf->db_password);

    globalConfig.urlConfig.sslShowUrl = NGX_STR_2_STD_STR(conf->ssl_show_base_url);
    globalConfig.urlConfig.nonSSLShowUrl = NGX_STR_2_STD_STR(conf->show_base_url);
    globalConfig.urlConfig.sslClickUrl = NGX_STR_2_STD_STR(conf->ssl_click_base_url);
    globalConfig.urlConfig.nonSSLClickUrl = NGX_STR_2_STD_STR(conf->click_base_url);

    std::string workdir = NGX_STR_2_STD_STR(conf->workdir);
    chdir(workdir.c_str());

    pid_t currentPid = getpid();
    std::cerr << "current pid:" << (int64_t)currentPid << std::endl;
    if (serviceLogger.use_count() != 0) {
        serviceLogger->stop();
    }

    serviceLogger = adservice::log::LogPusher::getLogger(MTTY_SERVICE_LOGGER, CONFIG_LOG, logConfig.localLoggerThreads,
                                                         !logConfig.kafkaLogEnable);
    serviceLogger->start();

    if (bidLogConfig.kafkaBroker.empty()) {
        bidServiceLogger = serviceLogger;
    } else {
        bidServiceLogger
            = adservice::log::LogPusher::getLogger(std::string("BID_") + MTTY_SERVICE_LOGGER, bidConfigKey,
                                                   bidLogConfig.localLoggerThreads, !bidLogConfig.kafkaLogEnable);
        bidServiceLogger->start();
    }
    serviceLogPtr = std::make_shared<AdServiceLog>(AdServiceLog::globalLoggingLevel);

    adSelectClient = std::make_shared<adservice::adselectv2::AdSelectClient>(globalConfig.adselectConfig.adselectNode);

    adservice::server::IpManager::init();
    adservice::corelogic::HandleBidQueryTask::init();
    adservice::corelogic::HandleShowQueryTask::loadTemplates();
    protocol::bidding::GuangyinBiddingHandler::loadStaticAdmTemplate();
    adservice::server::TypeTableManager::getInstance();
    adservice::server::ScenarioManager::getInstance();

    MT::common::traffic::TrafficControllProxy::instance_
        = std::make_shared<MT::common::traffic::TrafficControllProxy>(aerospikeClient);
    MT::common::traffic::TrafficControllProxy::instance_->start(std::cerr);

    adservice::utility::HttpClientProxy::instance_ = std::make_shared<adservice::utility::HttpClientProxy>();

    adservice::utility::AdSizeMap::getInstance();
    uniqueIdSeq
        = (adservice::utility::ip::ipStringToInt(adservice::utility::ip::getInterfaceIP("eth0")) & 0xFF) * currentPid;
    LOG_INFO << "using uniqueIdSeq:" << uniqueIdSeq;

    char cwd[256];
    getcwd(cwd, sizeof(cwd));
    std::cerr << "current working directory:" << cwd << std::endl;

    serviceInitialized = true;
}

void read_header(ngx_http_request_t * r, adservice::utility::HttpRequest & httpRequest)
{
    httpRequest.set(QUERYMETHOD, parseStr(r->method_name));
    httpRequest.set(URI, parseStr(r->uri));
    httpRequest.set(QUERYSTRING, parseStr(r->args));
    ngx_list_part_t * part = &r->headers_in.headers.part;
    ngx_table_elt_t * header = (ngx_table_elt_t *)part->elts;
    for (ngx_uint_t i = 0;; ++i) {
        if (i >= part->nelts) {
            if (part->next == nullptr) {
                break;
            }
            part = part->next;
            header = (ngx_table_elt_t *)part->elts;
            i = 0;
        }
        if (header[i].hash == 0) {
            continue;
        }
        // std::cerr<<"header:"<<parseKey(header[i])<<":"<<parseValue(header[i])<<std::endl;
        httpRequest.set(parseKey(header[i]), parseValue(header[i]));
    }
    // std::cerr<<"ip:"<<httpRequest.remote_addr()<<std::endl;
    std::stringstream cookiesstream;
    ngx_table_elt_t ** cookies = (ngx_table_elt_t **)r->headers_in.cookies.elts;
    for (ngx_uint_t i = 0; i < r->headers_in.cookies.nelts; ++i) {
        cookiesstream << parseValue(cookies[i]) << ";";
    }
    httpRequest.set(COOKIE, cookiesstream.str());
}

ngx_int_t build_response(ngx_http_request_t * r, adservice::utility::HttpResponse & httpResponse)
{
    r->headers_out.status = (ngx_uint_t)httpResponse.status();
    if (r->headers_out.status != 204 && httpResponse.get_body().empty()) { // http standard compromised to bussiness<-->
        httpResponse.set_body("\r");
    }
    const std::string & strResp = httpResponse.responseNeedGzip()
                                      ? adservice::utility::gzip::compress(httpResponse.get_body())
                                      : httpResponse.get_body();
    if (r->headers_out.status == 200) {
        r->headers_out.content_type.data = (uchar_t *)httpResponse.content_header().data();
        r->headers_out.content_type.len = httpResponse.content_header().length();
        r->headers_out.content_type_len = r->headers_out.content_type.len;
    }
    const std::map<std::string, std::string> headers = httpResponse.get_headers();
    for (auto & iter : headers) {
        if (iter.first == CONTENTTYPE) {
            continue;
        } else if (iter.first == CONTENTLENGTH) {
        }
        ngx_table_elt_t * h = (ngx_table_elt_t *)ngx_list_push(&r->headers_out.headers);
        if (h != nullptr) {
            h->hash = 1;
            h->key.data = (uchar_t *)iter.first.data();
            h->key.len = iter.first.length();
            h->value.data = (uchar_t *)iter.second.data();
            h->value.len = iter.second.length();
        }
    }

    ngx_int_t rc = ngx_http_send_header(r);
    if (rc != NGX_OK || r->headers_out.status == NGX_HTTP_NO_CONTENT) {
        return rc;
    }

    ngx_buf_t * b = (ngx_buf_t *)ngx_palloc(r->pool, sizeof(ngx_buf_t));
    if (b == nullptr) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Failed to allocate response buffer");
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    ngx_chain_t out;
    out.buf = b;
    out.next = nullptr;

    b->pos = (u_char *)strResp.c_str();
    b->last = (u_char *)strResp.c_str() + strResp.length();
    b->memory = 1;
    b->last_buf = 1;
    b->in_file = 0;
    return ngx_http_output_filter(r, &out);
}

void makeDebugRequest(adservice::utility::HttpRequest & request, protocol::debug::DebugRequest & debugRequest)
{
    request.set(QUERYMETHOD, debugRequest.originmethod());
    request.set(URI, debugRequest.originmodule());
    if (request.request_method() == "GET") {
        request.set(QUERYSTRING, debugRequest.requestdata());
    } else {
        request.set_post_data(debugRequest.requestdata());
    }
    // std::cerr << request.path_info() << " " << request.request_method() << std::endl;
}

void dispatchRequest(adservice::utility::HttpRequest & request, adservice::utility::HttpResponse & response)
{
    const std::string & queryPath = request.path_info();
    if (queryPath.find("bid") != std::string::npos) {
        adservice::corelogic::HandleBidQueryTask task(request, response);
        task.setLogger(bidServiceLogger);
        task();
    } else if (queryPath == "/v" || queryPath == "/s") {
        adservice::corelogic::HandleShowQueryTask task(request, response);
        task.setLogger(serviceLogger);
        task();
    } else if (queryPath == "/c") {
        adservice::corelogic::HandleClickQueryTask task(request, response);
        task.setLogger(serviceLogger);
        task();
    } else if (queryPath == "/m") {
        adservice::corelogic::HandleMappingQueryTask task(request, response);
        task.setLogger(serviceLogger);
        task();
    } else if (queryPath == "/t") {
        adservice::corelogic::HandleTraceTask task(request, response);
        task.setLogger(serviceLogger);
        task();
    } else if (queryPath == "/tool") {
        adservice::corelogic::HandleToolQueryTask task(request, response);
        task.setLogger(serviceLogger);
        task();
    } else {
        response.status(204);
        response.set_content_header("text/html");
    }
}

void after_read_post_data(ngx_http_request_t * r)
{
    adservice::utility::HttpRequest httpRequest;
    adservice::utility::HttpResponse httpResponse;
    read_header(r, httpRequest);
    if (r->request_body->temp_file == nullptr) {
        std::stringstream ss;
        ngx_buf_t * buf;
        ngx_chain_t * cl;
        cl = r->request_body->bufs;
        for (; NULL != cl; cl = cl->next) {
            buf = cl->buf;
            ss << std::string((const char *)buf->pos, (const char *)buf->last);
        }
        if (httpRequest.isGzip()) {
            httpRequest.set_post_data(adservice::utility::gzip::decompress(ss));
        } else {
            httpRequest.set_post_data(ss.str());
        }
        //        if (httpRequest.isResponseNeedGzip()) {
        //            httpResponse.setResponseGzip(true);
        //            httpResponse.set(CONTENTENCODING, "gzip");
        //        }
    }
    const std::string queryPath = httpRequest.path_info();
    if (queryPath.find("bid") != std::string::npos) {
        adservice::corelogic::HandleBidQueryTask task(httpRequest, httpResponse);
        task.setLogger(bidServiceLogger);
        task();
    } else if (queryPath == "/tool") {
        adservice::corelogic::HandleToolQueryTask task(httpRequest, httpResponse);
        task.setLogger(serviceLogger);
        task();
    } else if (queryPath == "/debug") { // debug module
        //根据debug 请求的包，将它解析成一个正常的请求，同时打上debug 标记
        //一旦打上debug标记所有debug级别以下的输出将被输出到 socket peer,因此debug模块可以跟踪整个流程
        protocol::debug::DebugRequest debugRequest;
        protocol::debug::DebugResponse debugResponse;
        bool parseResult = adservice::utility::serialize::getProtoBufObject(debugRequest, httpRequest.raw_post_data());
        if (!parseResult) {
            LOG_ERROR << "Debug Interface parse reqeust failed!!";
        } else {
            inDebugSession = true;
            debugSession = (void *)(&httpResponse);
            LOG_DEBUG << "start debug session";
            try {
                makeDebugRequest(httpRequest, debugRequest);
                dispatchRequest(httpRequest, httpResponse);
                debugResponse.set_respstatus(httpResponse.status());
                httpResponse.status(200);
                debugResponse.set_responsedata(httpResponse.get_body());
                debugResponse.set_debugmessage(httpResponse.get_debug_message());
                std::string outDebugResponse;
                if (!adservice::utility::serialize::writeProtoBufObject(debugResponse, &outDebugResponse)) {
                    std::cerr << "serialize debug response failed" << std::endl;
                }
                httpResponse.set_body(outDebugResponse);
            } catch (std::exception & e) {
                LOG_ERROR << "some error occured in Debug Session,e:" << e.what();
            }
            LOG_DEBUG << "end debug session";
            inDebugSession = false;
            debugSession = nullptr;
        }
    }
    ngx_http_finalize_request(r, build_response(r, httpResponse));
}

static ngx_int_t adservice_handler(ngx_http_request_t * r)
{
    globalLog = r->connection->log;
    if (!r->method & (NGX_HTTP_HEAD | NGX_HTTP_GET | NGX_HTTP_POST)) {
        return NGX_HTTP_NOT_ALLOWED;
    }

    if (!serviceInitialized) {
        LocationConf * conf = (LocationConf *)ngx_http_get_module_loc_conf(r, modAdservice);
        global_init(conf);
    }

    if (r->method == NGX_HTTP_POST) {
        ngx_int_t rc = ngx_http_read_client_request_body(r, after_read_post_data);
        if (rc >= NGX_HTTP_SPECIAL_RESPONSE) {
            return rc;
        }
        return NGX_DONE;
    }

    adservice::utility::HttpRequest httpRequest;
    read_header(r, httpRequest);

    adservice::utility::HttpResponse httpResponse;
    if (httpRequest.isResponseNeedGzip()) {
        httpResponse.setResponseGzip(true);
        httpResponse.set(CONTENTENCODING, "gzip");
    }
    dispatchRequest(httpRequest, httpResponse);

    return build_response(r, httpResponse);
}
