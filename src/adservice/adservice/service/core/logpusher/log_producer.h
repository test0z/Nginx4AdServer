//
// Created by guoze.lin on 16/2/26.
//

#ifndef ADCORE_LOG_PRODUCER_H
#define ADCORE_LOG_PRODUCER_H

#include <string>
#include "common/functions.h"
#include "common/constants.h"
#if defined(USE_ALIYUN_LOG)
#include <alibaba/SendResultONS.h>
#include <alibaba/Message.h>
#include <alibaba/ONSClientException.h>
#else
#include <exception>
#endif

namespace adservice{
    namespace log{

#if defined(USE_ALIYUN_LOG)
        typedef ons::Message    Message;
        typedef ons::ONSClientException LogClientException;
        class SendResult : public ons::SendResultONS{
            public:
                static const int SEND_OK = 0;
                static const int SEND_ERROR = 1;
            public:
                SendResult(){}
                SendResult(const ons::SendResultONS& resultOns):ons::SendResultONS(resultOns){}
                SendResult(int error):res(error){}
                int getResult(){return res;}
            private:
                int res;
        };
#else
        enum SendResult{
            SEND_OK,
            SEND_ERROR
        };

        struct Message{
            std::string topics;
            std::string bytes;
            Message(){}
            Message(const std::string& t,const std::string& msg):topics(t),bytes(msg){}
            Message& operator=(const Message& m){
                    topics = m.topics;
                    bytes = m.bytes;
                    return *this;
            }
        };

        class LogClientException : public std::exception{
        public:
            LogClientException() _GLIBCXX_USE_NOEXCEPT {}
            LogClientException(const std::string& str,int error) _GLIBCXX_USE_NOEXCEPT :message(str),errorCode(error){}
            const char* GetMsg() const { return message.c_str();}
            const char* what() const _GLIBCXX_USE_NOEXCEPT {return message.c_str();}
            int GetError() const {return errorCode;}
        private:
            std::string message;
            int errorCode;
        };

#endif

        class LogProducer{
        public:
            virtual ~LogProducer(){}
            virtual void configure(){}
            virtual void start() = 0;
            virtual SendResult send(Message& m) = 0;
            virtual SendResult send(Message&& m) = 0;
            virtual void shutdown() = 0;
        };

        enum LogProducerType : char{
            LOG_DEFAULT,
            LOG_ALIYUN,
            LOG_KAFKA
        };

        class LogProducerFactory{
        public:
            static LogProducer* createProducer(LogProducerType type,const std::string& loggerName,const std::string& logConfigKey = CONFIG_LOG);
        };

    }
}

#endif //ADCORE_LOG_PRODUCER_H
