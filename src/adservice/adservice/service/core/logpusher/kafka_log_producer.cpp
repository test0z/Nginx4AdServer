//
// Created by guoze.lin on 16/2/26.
//
#include "kafka_log_producer.h"
#include "core/config_types.h"
#include "log_pusher.h"
#include "logging.h"
#include <cstdlib>
#include <exception>
#include <mtty/constants.h>

extern GlobalConfig globalConfig;

namespace adservice {
namespace log {

    using namespace std;
    using namespace adservice::server;
    using namespace RdKafka;

    void LogDeliverReportCb::dr_cb(RdKafka::Message & message)
    {
        if (message.err() != ERR_NO_ERROR && !needRecover) { // kafka 发送发生错误
            LOG_ERROR << "error occured in kafka,err" << message.errstr() << " errCode:" << message.err();
            LogPusherPtr logPusher = LogPusher::getLogger(MTTY_SERVICE_LOGGER);
            logPusher->setWorkMode(true);
            const char * payload = (const char *)message.payload();
#if defined(USE_ALIYUN_LOG)
            log::Message msg(message.topic_name(), "", std::string(payload, payload + message.len()));
#else
            log::Message msg(message.topic_name(), std::string(payload, payload + message.len()));
#endif
            // logPusher->startRemoteMonitor(msg);
            needRecover = true;
        } else if (message.err() == ERR_NO_ERROR && needRecover) { // kafka 错误恢复
            LOG_INFO << "kafka error recover,continue to work";
            needRecover = false;
            LogPusherPtr logPusher = LogPusher::getLogger(MTTY_SERVICE_LOGGER);
            logPusher->setWorkMode(false);
        }
    }

    void LogEventCb::event_cb(Event & event)
    {
        switch (event.type()) {
        case Event::EVENT_ERROR:
            LOG_ERROR << "error occured in kafka,broker:" << event.broker_name() << " " << event.str()
                      << ",errcode:" << event.err();
            break;
        case Event::EVENT_LOG:
            LOG_INFO << "event log in kafka," << event.str();
            break;
        case Event::EVENT_THROTTLE:
            LOG_INFO << "event trottle from kafka broker,broker:" << event.broker_name() << " " << event.str();
            break;
        default:
            break;
        }
    }

    void KafkaLogProducer::configure()
    {
        // todo: modified with configmanager
        auto configIter = globalConfig.logConfig.find(logConfigKey);
        if (configIter == globalConfig.logConfig.end()) {
            LOG_ERROR << "Log Config key " << logConfigKey << " not found!!";
            throw "log config key not found";
        }

        LogConfig & logConfig = configIter->second;
        topicName = logConfig.kafkaTopic;
        logKey = logConfig.kafkaKey;
        kafkaPartition = logConfig.partitionCnt;
        std::string brokers = logConfig.kafkaBroker;
        RdKafka::Conf * conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
        RdKafka::Conf * tconf = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);
        std::string errstr;
        RdKafka::Conf::ConfResult res;
        if ((res = conf->set("metadata.broker.list", brokers, errstr)) != RdKafka::Conf::CONF_OK
            || (res = conf->set("event_cb", &eventCb, errstr)) != RdKafka::Conf::CONF_OK
            || (res = conf->set("dr_cb", &drCb, errstr)) != RdKafka::Conf::CONF_OK
            || (res = conf->set("queue.buffering.max.messages", logConfig.kafkaMQMaxSize, errstr))
                   != RdKafka::Conf::CONF_OK
            || (res = conf->set("message.send.max.retries", "3", errstr)) != RdKafka::Conf::CONF_OK
            || (res = conf->set("retry.backoff.ms", "500", errstr)) != RdKafka::Conf::CONF_OK) {
            LOG_ERROR << "error occured when configuring kafka log producer," << errstr;
            throw LogClientException(errstr, -1);
        }
        producer = RdKafka::Producer::create(conf, errstr);
        if (!producer) {
            LOG_ERROR << "error occured when configuring kafka log producer" << errstr;
            throw LogClientException(errstr, -1);
        }
        topic = RdKafka::Topic::create(producer, topicName, tconf, errstr);
        if (!topic) {
            LOG_ERROR << "error occured when configuring kafka log producer" << errstr;
            throw LogClientException(errstr, -1);
        }
    }

    static const std::string randomKey[26] = { "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m",
                                               "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z" };

    SendResult KafkaLogProducer::send(Message & msg)
    {
#if defined(USE_ALIYUN_LOG)
        const char * bytes = msg.getBody().c_str();
        int len = msg.getBody().length();
#else
        const char * bytes = msg.bytes.c_str();
        int len = msg.bytes.length();
#endif
        // int randIdx = random() % 26;
        int partition = kafkaPartition > 0 ? random() % kafkaPartition : RdKafka::Topic::PARTITION_UA;
        RdKafka::ErrorCode resp = producer->produce(topic, partition /*RdKafka::Topic::PARTITION_UA*/,
                                                    RdKafka::Producer::RK_MSG_COPY, (void *)bytes, len, NULL, NULL);
        if (resp == RdKafka::ErrorCode::ERR_NO_ERROR) {
            return SendResult::SEND_OK;
        } else {
            LOG_ERROR << "kafka produce log error,error code:" << resp;
            return SendResult::SEND_ERROR;
        }
    }

    SendResult KafkaLogProducer::send(Message && msg)
    {
#if defined(USE_ALIYUN_LOG)
        const char * bytes = msg.getBody().c_str();
        int len = msg.getBody().length();
#else
        const char * bytes = msg.bytes.c_str();
        int len = msg.bytes.length();
#endif
        //        int randIdx = random() % 26;
        int partition = kafkaPartition > 0 ? random() % kafkaPartition : RdKafka::Topic::PARTITION_UA;
        RdKafka::ErrorCode resp = producer->produce(topic, partition /*RdKafka::Topic::PARTITION_UA*/,
                                                    RdKafka::Producer::RK_MSG_COPY, (void *)bytes, len, NULL, NULL);
        if (resp == RdKafka::ErrorCode::ERR_NO_ERROR) {
            return SendResult::SEND_OK;
        } else {
            LOG_ERROR << "kafka produce log error,error code:" << resp;
            return SendResult::SEND_ERROR;
        }
    }

    void * kafkaTimer(void * param)
    {
        KafkaTimerParam * _param = (KafkaTimerParam *)param;
        RdKafka::Producer * p = _param->producer;
        while (_param->run) {
            p->poll(1000);
        }
        return NULL;
    }

    void KafkaLogProducer::start()
    {
        timerParam.producer = producer;
        timerParam.run = true;
        if (pthread_create(&kafkaTimerThread, NULL, &kafkaTimer, &timerParam)) {
            LOG_ERROR << "create kafka log timer error";
            return;
        }
        pthread_detach(kafkaTimerThread);
    }

    void KafkaLogProducer::shutdown()
    {
        timerParam.run = false;
        RdKafka::wait_destroyed(5000);
    }
}
}
