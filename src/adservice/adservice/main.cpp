#include <mutex>
#include <ostream>
#include <string>

#include <sys/types.h>

extern "C" {
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_hash.h>
#include <ngx_http.h>
#include <ngx_http_request.h>
#include <ngx_string.h>
}

#include "common/constants.h"
#include "core/adselectv2/ad_select_client.h"
#include "core/config_types.h"
#include "core/core_ip_manager.h"
#include "core/core_threadlocal_manager.h"
#include "core/logic/bid_query_task.h"
#include "core/logic/click_query_task.h"
#include "core/logic/show_query_task.h"
#include "core/logic/trace_task.h"
#include "core/logpusher/log_pusher.h"
#include "logging.h"
#include "protocol/debug/debug.pb.h"
#include "protocol/guangyin/guangyin_bidding_handler.h"
#include "utility/aero_spike.h"
#include "utility/utility.h"
#include <boost/algorithm/string.hpp>
#include <unistd.h>

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
    // working directory
    ngx_str_t workdir;
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

static ngx_command_t commands[] = { COMMAND_ITEM("logging_level", LocationConf, logginglevel, parseConfNum),
                                    COMMAND_ITEM("kafka_broker", LocationConf, kafkabroker, parseConfStr),
                                    COMMAND_ITEM("kafka_topic", LocationConf, kafkatopic, parseConfStr),
                                    COMMAND_ITEM("kafka_key", LocationConf, kafkakey, parseConfStr),
                                    COMMAND_ITEM("kafka_mqmaxsize", LocationConf, kafkamqmaxsize, parseConfStr),
                                    COMMAND_ITEM("kafka_logger_enable", LocationConf, kafkaloggerenable, parseConfNum),
                                    COMMAND_ITEM("local_logger_thread", LocationConf, localloggerthreads, parseConfNum),
                                    COMMAND_ITEM("adselect_entry", LocationConf, adselectentry, parseConfStr),
                                    COMMAND_ITEM("adselect_timeout", LocationConf, adselecttimeout, parseConfStr),
                                    COMMAND_ITEM("as_node", LocationConf, asnode, parseConfStr),
                                    COMMAND_ITEM("as_namespace", LocationConf, asnamespace, parseConfStr),
                                    COMMAND_ITEM("workdir", LocationConf, workdir, parseConfStr),
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
    conf->localloggerthreads = NGX_CONF_UNSET_UINT;
    ngx_str_null(&conf->adselectentry);
    ngx_str_null(&conf->adselecttimeout);
    ngx_str_null(&conf->asnode);
    ngx_str_null(&conf->asnamespace);
    ngx_str_null(&conf->workdir);
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
    ngx_conf_merge_uint_value(conf->localloggerthreads, prev->localloggerthreads, 3);
    ngx_conf_merge_str_value(conf->adselectentry, prev->adselectentry, "");
    ngx_conf_merge_str_value(conf->adselecttimeout, prev->adselecttimeout, "0:15|21:-1|98:-1|99:-1");
    ngx_conf_merge_str_value(conf->asnode, prev->asnode, "");
    ngx_conf_merge_str_value(conf->asnamespace, prev->asnamespace, "mtty");
    ngx_conf_merge_str_value(conf->workdir, prev->workdir, "/usr/local/nginx/sbin/");
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
AdServiceLogPtr serviceLogPtr = nullptr;
adservice::adselectv2::AdSelectClientPtr adSelectClient;
ngx_log_t * globalLog;
__thread bool inDebugSession = false;
__thread void * debugSession = nullptr;

#define NGX_STR_2_STD_STR(str) std::string((const char *)str.data, (const char *)str.data + str.len)
#define NGX_BOOL(b) (b == TRUE)

void parseConfigAeroSpikeNode(const std::string & asNode, AerospikeConfig & config)
{
    size_t prevPos = 0, nextPos;
    while ((nextPos = asNode.find(",", prevPos)) != std::string::npos) {
        size_t portPos = 0;
        if ((portPos = asNode.find(":", prevPos)) != std::string::npos) {
            config.connections.push_back(AerospikeConfig::Connection(
                std::string(asNode.data() + prevPos, asNode.data() + portPos), std::stoi(asNode.data() + portPos + 1)));
        } else {
            config.connections.push_back(
                AerospikeConfig::Connection(std::string(asNode.data() + prevPos, asNode.data() + nextPos), 3000));
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
    AdServiceLog::globalLoggingLevel = (LoggingLevel)globalConfig.serverConfig.loggingLevel;
}

static void global_init(LocationConf * conf)
{
    globalMutex.lock();
    if (serviceInitialized)
        return;
    globalConfig.serverConfig.loggingLevel = (int)conf->logginglevel;
    setGlobalLoggingLevel(globalConfig.serverConfig.loggingLevel);
    globalConfig.logConfig.kafkaBroker = NGX_STR_2_STD_STR(conf->kafkabroker);
    globalConfig.logConfig.kafkaKey = NGX_STR_2_STD_STR(conf->kafkakey);
    globalConfig.logConfig.kafkaTopic = NGX_STR_2_STD_STR(conf->kafkatopic);
    globalConfig.logConfig.kafkaMQMaxSize = NGX_STR_2_STD_STR(conf->kafkamqmaxsize);
    globalConfig.logConfig.kafkaLogEnable = NGX_BOOL(conf->kafkaloggerenable);
    globalConfig.logConfig.localLoggerThreads = conf->localloggerthreads;
    globalConfig.adselectConfig.adselectNode = NGX_STR_2_STD_STR(conf->adselectentry);
    parseConfigAdselectTimeout(NGX_STR_2_STD_STR(conf->adselecttimeout), globalConfig.adselectConfig.adselectTimeout);
    globalConfig.aerospikeConfig.nameSpace = NGX_STR_2_STD_STR(conf->asnamespace);
    std::string asNode = NGX_STR_2_STD_STR(conf->asnode);
    parseConfigAeroSpikeNode(asNode, globalConfig.aerospikeConfig);
    std::string workdir = NGX_STR_2_STD_STR(conf->workdir);
    chdir(workdir.c_str());
    pid_t currentPid = getpid();
    std::cerr << "current pid:" << (int64_t)currentPid << std::endl;
    if (serviceLogger.use_count() != 0) {
        serviceLogger->stop();
    }
    serviceLogger = adservice::log::LogPusher::getLogger(MTTY_SERVICE_LOGGER,
                                                         CONFIG_LOG,
                                                         globalConfig.logConfig.localLoggerThreads,
                                                         !globalConfig.logConfig.kafkaLogEnable);
    serviceLogger->start();

    serviceLogPtr = std::make_shared<AdServiceLog>(AdServiceLog::globalLoggingLevel);

    adSelectClient = std::make_shared<adservice::adselectv2::AdSelectClient>(globalConfig.adselectConfig.adselectNode);

    adservice::server::IpManager::init();
    adservice::corelogic::HandleBidQueryTask::init();
    adservice::corelogic::HandleShowQueryTask::loadTemplates();
    protocol::bidding::GuangyinBiddingHandler::loadStaticAdmTemplate();

    char cwd[256];
    getcwd(cwd, sizeof(cwd));
    std::cerr << "current working directory:" << cwd << std::endl;

    serviceInitialized = true;
    globalMutex.unlock();
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
        cookiesstream << parseKey(cookies[i]) << "=" << parseValue(cookies[i]) << ";";
    }
    httpRequest.set(COOKIE, cookiesstream.str());
}

ngx_int_t build_response(ngx_http_request_t * r, adservice::utility::HttpResponse & httpResponse)
{
    r->headers_out.status = (ngx_uint_t)httpResponse.status();
    if (r->headers_out.status != 204 && httpResponse.get_body().empty()) { // http standard compromised to bussiness<-->
        httpResponse.set_body("\r");
    }
    const std::string & strResp = httpResponse.get_body();
    if (r->headers_out.status == 200) {
        r->headers_out.content_type.data = (uchar_t *)httpResponse.content_header().data();
        r->headers_out.content_type.len = httpResponse.content_header().length();
        r->headers_out.content_type_len = r->headers_out.content_type.len;
    }
    const std::map<std::string, std::string> headers = httpResponse.get_headers();
    for (auto & iter : headers) {
        if (iter.first == CONTENTTYPE) {
            continue;
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
    if (rc != NGX_OK) {
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
    request.set(QUERYMETHOD, debugRequest.originmodule());
    request.set(URI, debugRequest.originmethod());
    if (request.request_method() == "GET") {
        request.set(QUERYSTRING, debugRequest.requestdata());
    } else {
        request.set_post_data(debugRequest.requestdata());
    }
}

void dispatchRequest(adservice::utility::HttpRequest & request, adservice::utility::HttpResponse & response)
{
    const std::string & queryPath = request.path_info();
    if (queryPath.find("bid") != std::string::npos) {
        adservice::corelogic::HandleBidQueryTask task(request, response);
        task.setLogger(serviceLogger);
        task();
    } else if (queryPath == "/v" || queryPath == "/s") {
        adservice::corelogic::HandleShowQueryTask task(request, response);
        task.setLogger(serviceLogger);
        task();
    } else if (queryPath == "/c") {
        adservice::corelogic::HandleClickQueryTask task(request, response);
        task.setLogger(serviceLogger);
        task();
    } else if (queryPath == "/t") {
        adservice::corelogic::HandleTraceTask task(request, response);
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
        httpRequest.set_post_data(ss.str());
    }
    const std::string queryPath = httpRequest.path_info();
    if (queryPath.find("bid") != std::string::npos) {
        adservice::corelogic::HandleBidQueryTask task(httpRequest, httpResponse);
        task.setLogger(serviceLogger);
        task();
    } else if (queryPath == "debug") { // debug module
        //根据debug 请求的包，将它解析成一个正常的请求，同时打上debug 标记
        //一旦打上debug标记所有debug级别以下的输出将被输出到 socket peer,因此debug模块可以跟踪整个流程
        protocol::debug::DebugRequest debugRequest;
        bool parseResult = adservice::utility::serialize::getProtoBufObject(debugRequest, httpRequest.raw_post_data());
        if (!parseResult) {
            LOG_ERROR << "Debug Interface parse reqeust failed!!";
        } else {
            inDebugSession = true;
            debugSession = (void *)(&httpResponse);
            try {
                makeDebugRequest(httpRequest, debugRequest);
                dispatchRequest(httpRequest, httpResponse);
            } catch (std::exception & e) {
                LOG_ERROR << "some error occured in Debug Session,e:" << e.what();
            }
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
    dispatchRequest(httpRequest, httpResponse);

    return build_response(r, httpResponse);
}
