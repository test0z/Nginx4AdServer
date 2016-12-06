//
// Created by guoze.lin on 16/4/11.
//
#include "common/functions.h"
#include "core/adselectv2/ad_select_client.h"
#include "core/config_types.h"
#include "core/logpusher/log_pusher.h"
#include "logging.h"
#include "utility/utility.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/tmpdir.hpp>
#include <boost/serialization/map.hpp>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <functional>
#include <map>
#include <string>
#include <sys/stat.h>
#define PROGRESS_FILE "progress.bin"
#define COUNTER_FILE "counter.bin"

using namespace std;
using namespace adservice::utility::serialize;

adservice::adselectv2::AdSelectClient * adSelectClient;
adservice::log::LogPusherPtr serviceLogger = nullptr;
GlobalConfig globalConfig;
bool inDebugSession = false;
void * debugSession = nullptr;

std::map<std::string, int> progressMap;
typedef std::map<std::string, int>::iterator ProgressMapIter;

std::map<std::string, int> counterMap;
typedef std::map<std::string, int>::iterator CounterMapIter;

typedef std::function<bool(const char *, int)> SyncLogCallback;

bool init()
{
    ifstream ifs1(PROGRESS_FILE, ios_base::in);
    if (!ifs1.good()) {
        LOG_DEBUG << "init,can't open progress file";
        return false;
    }
    boost::archive::text_iarchive progress(ifs1);
    progress >> progressMap;
    ifstream ifs2(COUNTER_FILE, ios_base::in);
    if (!ifs2.good()) {
        LOG_DEBUG << "init,can't open counter file";
        return false;
    }
    boost::archive::text_iarchive counter(ifs2);
    counter >> counterMap;
    ifs1.close();
    ifs2.close();
    return true;
}

void finalize()
{
    ofstream ofs1(PROGRESS_FILE, ios_base::out | ios_base::trunc);
    if (!ofs1.good())
        throw "finalize,can't open progress file";
    boost::archive::text_oarchive progress(ofs1);
    progress << progressMap;
    ofstream ofs2(COUNTER_FILE, ios_base::out | ios_base::trunc);
    if (!ofs2.good())
        throw "finalize,can't open counter file";
    boost::archive::text_oarchive counter(ofs2);
    counter << counterMap;
    ofs1.close();
    ofs2.close();
}

bool isHead(char * p)
{
    if (p[0] != 'm' && p[1] != 't') {
        return false;
    }
    p += 2;
    while (*p != '^' && *p != '\0') {
        if (48 > *p || *p > 57) {
            return false;
        }
        p++;
    }
    if (*p == '^') {
        return true;
    }
    return false;
}

void readFile(const char * filename, const SyncLogCallback & logCallback = SyncLogCallback())
{
    std::string file = filename;
    ProgressMapIter iter = progressMap.find(file);
    int fileOffset = 0;
    if (iter == progressMap.end()) {
        progressMap[file] = 0;
        fileOffset = 0;
    } else {
        fileOffset = iter->second;
    }
    CounterMapIter counterIter = counterMap.find(file);
    int counter = 0;
    if (counterIter == counterMap.end()) {
        counterMap[file] = 0;
        counter = 0;
    } else {
        counter = counterMap[file];
    }
    FILE * fp = fopen(filename, "rb");
    if (!fp) {
        LOG_DEBUG << "error open file," << filename;
        return;
    }
    fseek(fp, fileOffset, SEEK_SET);
    while (!feof(fp)) {
        char logBuffer[2048];
        int len = fread(logBuffer, 1, sizeof(logBuffer) - 1, fp);
        logBuffer[len] = '\0';
        char *begin = NULL, *end = NULL;
        for (int i = 0; i < len; i++) {
            if (logBuffer[i] == '^' && begin == NULL) {
                begin = logBuffer + i + 1;
            } else if (begin != NULL && isHead(logBuffer + i)) {
                end = logBuffer + i - 1;
                // LOG_DEBUG<<"next:"<<logBuffer+i;
                break;
            }
        }
        if (feof(fp) || end <= begin) {
            LOG_DEBUG << "begin," << (int)(begin - logBuffer) << ",end:" << (int)(end - logBuffer)
                      << ",logBuffer:" << logBuffer;
            break;
        }
        fseek(fp, fileOffset + end - logBuffer + 1, SEEK_SET);
        if (logCallback) {
            bool result = logCallback(begin, end - begin + 1);
            if (!result) {
                LOG_DEBUG << "logCallback failed,len:" << end - begin + 1 << ",file offset:" << fileOffset;
                LOG_DEBUG << "logBuffer:" << logBuffer;
                // fileOffset = ftell(fp);
                // break;
            }
        }
        fileOffset = ftell(fp);
        counter++;
    }
    fclose(fp);
    // printf("fileOffset:%d,counter:%d\n",fileOffset,counter);
    progressMap[file] = fileOffset;
    counterMap[file] = counter;
}

using namespace adservice::log;
using namespace adservice::server;
KafkaLogProducer * logProducer = NULL;

bool checkDataValid(const char * data, int size)
{
    try {
        protocol::log::LogItem logItem;
        getAvroObject(logItem, (const uint8_t *)data, size);
        return true;
    } catch (avro::Exception & e) {
        LOG_DEBUG << "check parsing avro object failed";
    } catch (std::exception & e) {
        LOG_DEBUG << "error occured in checkDataValid," << e.what();
    }
    return false;
}

bool pushLog2Kafka(const char * data, int size)
{
    try {
        if (!checkDataValid(data, size))
            return false;
        std::shared_ptr<std::string> logString = std::make_shared<std::string>(data, data + size);
        serviceLogger->push(logString);
        return true;
    } catch (LogClientException & e) {
        LOG_DEBUG << "LogClient error:" << e.GetMsg() << " errorcode:" << e.GetError();
    } catch (std::exception & e) {
        LOG_DEBUG << "error occured in LogPushClickTask,err:" << e.what();
    }
    return false;
}

extern std::string getLogItemString(protocol::log::LogItem & log);

bool verifydata(const char * data, int size)
{
    try {
        protocol::log::LogItem logItem;
        getAvroObject(logItem, (const uint8_t *)data, size);
        std::string str = getLogItemString(logItem);
        printf("%s\n", str.c_str());
        return true;
    } catch (avro::Exception & e) {
        LOG_DEBUG << "error in parsing avro log object" << e.what();
    } catch (std::exception & e) {
        LOG_DEBUG << "error in verifydata," << e.what();
    }
    return false;
}

int main(int argc, const char ** argv)
{
    if (argc != 3) {
        printf("usage:locallog_collector [DIR] [OPERATION]\n");
        return 0;
    }
    const char * operation = argv[2];
    SyncLogCallback syncCB;
    if (strcmp(operation, "check") == 0) {
        syncCB = std::bind(&verifydata, std::placeholders::_1, std::placeholders::_2);
    } else if (strcmp(operation, "sync") == 0) {
        globalConfig.logConfig.kafkaBroker = "192.168.2.52";
        globalConfig.logConfig.kafkaMQMaxSize = "10000";
        globalConfig.logConfig.kafkaTopic = "mt-log";
        globalConfig.logConfig.localLoggerThreads = 3;
        globalConfig.logConfig.kafkaLogEnable = true;
        serviceLogger = adservice::log::LogPusher::getLogger(MTTY_SERVICE_LOGGER, CONFIG_LOG, 3, false);
        serviceLogger->start();
        syncCB = std::bind(&pushLog2Kafka, std::placeholders::_1, std::placeholders::_2);
    }
    try {
        if (!init()) {
            finalize();
            init();
        }
        const char * targetDir = argv[1];
        DIR * dir = opendir(targetDir);
        if (!dir) {
            printf("can't open dir");
            return 0;
        }
        struct dirent * dirent = NULL;
        // DebugMessage("collecting dir ", targetDir);
        while ((dirent = readdir(dir)) != NULL) {
            if (strcmp(".", dirent->d_name) == 0 || strcmp("..", dirent->d_name) == 0)
                continue;
            char buffer[1024];
            sprintf(buffer, "%s/%s", targetDir, dirent->d_name);
            LOG_DEBUG << "collecting file " << buffer;
            readFile(buffer, syncCB);
        }
        for (CounterMapIter iter = counterMap.begin(); iter != counterMap.end(); iter++) {
            printf("%s - %d \n", iter->first.c_str(), iter->second);
        }
        finalize();
    } catch (const char * errstr) {
        LOG_DEBUG << "exception caught:" << errstr;
    } catch (std::exception & e) {
        LOG_DEBUG << "error occured:" << e.what();
    }
    if (logProducer != NULL) {
        serviceLogger->stop();
    }
    return 0;
}
