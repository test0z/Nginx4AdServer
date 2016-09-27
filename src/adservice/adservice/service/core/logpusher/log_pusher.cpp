//
// Created by guoze.lin on 16/2/14.
//

#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>
#include "log_pusher.h"
#include "utility/utility.h"
#include "core/core_threadlocal_manager.h"
#include "common/atomic.h"
#include "logging.h"

namespace adservice{
    namespace log{

        using namespace adservice::utility::escape;
        using namespace Logging;

        struct spinlock LogPusher::lock={0};
        LogPusherMap LogPusher::logMap;
        std::string LogPusher::localLogFilePrefix=".";

        LogProducer* LogProducerFactory::createProducer(LogProducerType type,const std::string& loggerName,const std::string& logConfigKey) {
            switch(type){
                case LOG_KAFKA:
                    LOG_INFO<<"using kafka log";
                    return new KafkaLogProducer(loggerName,logConfigKey);
                    break;
                default:
                    LOG_INFO<<"using local log";
                    return NULL;
            }
        }


        struct LogPushTask{
            LogPushTask(LogProducer* p,std::shared_ptr<adservice::types::string>& l):producer(p),log(l){}
            LogPushTask(LogProducer* p,std::shared_ptr<adservice::types::string>&& l):producer(p),log(l){}
            void operator()(){
                Message msg("",*(log.get()));
                try{
                    SendResult sendResult = producer->send(msg);
                    if(sendResult==SendResult::SEND_ERROR){
                        LogPusherPtr logger = LogPusher::getLogger(MTTY_SERVICE_LOGGER);
                        logger->setWorkMode(true);
                        logger->startRemoteMonitor(msg);
                    }
                }catch(LogClientException& e){
                    LOG_ERROR << "LogClient error:" << e.GetMsg() << " errorcode:" << e.GetError();
                    LogPusherPtr logger = LogPusher::getLogger(MTTY_SERVICE_LOGGER);
                    logger->setWorkMode(true);
                    logger->startRemoteMonitor(msg);
                }catch(std::exception& e){
                    LOG_ERROR << "error occured in LogPushClickTask,err:"<<e.what();
                }
            }
            LogProducer* producer;
            std::shared_ptr<adservice::types::string> log;
        };

        struct LogPushLocalThreadData{
            FILE* fp;
            long lastTime;
            int logCnt;
            LogPushLocalThreadData():fp(NULL),lastTime(0),logCnt(0){}
            ~LogPushLocalThreadData(){
                if(fp!=NULL){
                    fclose(fp);
                }
            }
            static void destructor(void* ptr){
                if(ptr){
                    delete ((LogPushLocalThreadData*)ptr);
                }
            }
        };

#define HOUR_SECOND     3600
        struct LogPushLocalTask{
            LogPushLocalTask(std::shared_ptr<adservice::types::string>& l):log(l){}
            LogPushLocalTask(std::shared_ptr<adservice::types::string>&& l):log(l){}
            void operator()(){
                pthread_t thread = pthread_self();
                LogPushLocalThreadData* data = (LogPushLocalThreadData*)ThreadLocalManager::getInstance().get(thread);
                if(data==NULL){
                    data = new LogPushLocalThreadData;
                    ThreadLocalManager::getInstance().put(thread,data,&LogPushLocalThreadData::destructor);
                }
                FILE* fp = data->fp;
                long curTime = utility::time::getCurrentTimeStamp();
                bool expired = false;
                if(fp==NULL||(expired=data->lastTime<curTime-HOUR_SECOND)){
                    if(expired){
                        fclose(fp);
                        fp = NULL;
                        LOG_INFO<<"hourly log cnt:"<<data->logCnt<<" of thread "<<(long)thread;
                        data->logCnt=0;
                    }
                    std::string logfolder = LogPusher::getLocalLogFilePrefix()+"/logs";
                    if(access(logfolder.data(),F_OK)==-1){
                        if(mkdir(logfolder.data(),S_IRWXU|S_IRWXG)<0){
                            LOG_ERROR << "dir log can not be created!";
                            return;
                        }
                    }
                    time_t currentTime;
                    ::time(&currentTime);
                    tm* ltime = localtime(&currentTime);
                    char dirname[1024];
                    sprintf(dirname,"%s/%d%02d%02d%02d",logfolder.data(),1900+ltime->tm_year,ltime->tm_mon+1,ltime->tm_mday,ltime->tm_hour);
                    if(access(dirname,F_OK)==-1){
                        if(mkdir(dirname,S_IRWXU|S_IRWXG)<0){
                            LOG_ERROR << "dir "<<dirname<<" can not be created!";
                        }
                    }
                    char filename[1024];
                    sprintf(filename, "%s/click.%lu.log", dirname, thread);
                    fp = fopen(filename,"wb+");
                    if(fp==NULL){
                        LOG_ERROR << "file " << filename<<" can not be opened!";
                        return;
                    }
                    data->lastTime = utility::time::getTodayStartTime()+ltime->tm_hour*HOUR_SECOND;
                    data->fp = fp;
                }
                char flag[20]={'\0'};
                sprintf(flag,"mt%lu^",log->length());
                fwrite(flag,strlen(flag),1,fp);
                fwrite(log->c_str(),log->length(),1,fp);
                data->logCnt++;
            }
            std::shared_ptr<adservice::types::string> log;
        };


        LogPusherPtr LogPusher::getLogger(const std::string& name,const std::string& logConfigKey,int ifnodefineThreads,bool logLocal){ //fixme: std::map is not thread-safe,risk still holds
            LogPusherMapAccessor acc;
            if(!logMap.find(acc,name)){
                spinlock_lock(&lock);
                if(!logMap.find(acc,name)) {
                    logMap.insert(acc,name);
                    acc->second = std::make_shared<LogPusher>(name.c_str(), ifnodefineThreads, logLocal,logConfigKey);
                }
                spinlock_unlock(&lock);
            }
            return acc->second;
        }

        void LogPusher::removeLogger(const std::string& name){
            logMap.erase(name);
        }

        void LogPusher::push(std::shared_ptr<adservice::types::string>& logstring){
            if(!modeLocal) {
                //由于现在使用的kafka client api有自己的消息队列机制,所以不需要走logpusher内部消息队列
                SendResult sendResult = producer->send(Message(DEFAULT_KAFKA_TOPIC,*(logstring.get())));
                if(sendResult==SendResult::SEND_ERROR){
                    this->setWorkMode(true);
                    this->startRemoteMonitor(Message(DEFAULT_KAFKA_TOPIC,*(logstring.get())));
                }
            }else{
                executor.run(std::bind(LogPushLocalTask(logstring)));
            }
        }

        void LogPusher::push(std::shared_ptr<adservice::types::string>&& logstring){
            if(!modeLocal) {
                SendResult sendResult =producer->send(Message(DEFAULT_KAFKA_TOPIC,*(logstring.get())));
                if(sendResult==SendResult::SEND_ERROR){
                    this->setWorkMode(true);
                    this->startRemoteMonitor(Message(DEFAULT_KAFKA_TOPIC,*(logstring.get())));
                }
            }else{
                executor.run(std::bind(LogPushLocalTask(logstring)));
            }
        }

        struct RemoteMonitorThreadParam{
            LogProducer* producer;
            log::Message msg;
            int started;
            RemoteMonitorThreadParam():started(0){}
            RemoteMonitorThreadParam(LogProducer* p,const log::Message& m):producer(p),msg(m),started(0){}
            void init(LogProducer* p,const log::Message& m){
                producer = p;
                msg = m;
            }
        };

        void* monitorRemoteLog(void* param){
            RemoteMonitorThreadParam* _param = (RemoteMonitorThreadParam*)param;
            LogProducer* producer = _param->producer;
            Message& msg = _param->msg;
            int retryTimes = 0;
            while(true) {
                retryTimes++;
                try {
                    SendResult result = producer->send(msg);
                    if(result == SendResult::SEND_ERROR){
                        if(retryTimes%30==0)
                            LOG_ERROR<<"log client error still exists";
                    }else{
                        LogPusher::getLogger(MTTY_SERVICE_LOGGER)->setWorkMode(false);
                        LOG_INFO<<"log client error recover,continue with remote logging";
                        break;
                    }
                    sleep(5);
                    if(!LogPusher::getLogger(MTTY_SERVICE_LOGGER)->getWorkMode()){
                        LOG_INFO<<"log client error recover,continue with remote logging";
                        break;
                    }
                } catch (LogClientException &e) {
                    if(retryTimes%30==0)
                        LOG_ERROR<<"log client error still exists,error:"<<e.GetError();
                }
                sleep(2);
            }
            _param->started = 0;
            return NULL;
        }

        void LogPusher::startRemoteMonitor(const log::Message& msg) { //fixme:fix multi log problem by defining param as a class member
            static RemoteMonitorThreadParam param;
            if(param.started)
                return;
            if(!ATOM_CAS(&param.started,0,1))
                return;
            param.init(producer,msg);
            pthread_t monitorThread;
            if(pthread_create(&monitorThread,NULL,&monitorRemoteLog,&param)){
                LOG_ERROR<<"create remote log monitor error";
                return;
            }
            pthread_detach(monitorThread);
        }


    }
}
