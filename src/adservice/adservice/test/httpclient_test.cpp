#include "utility/utility.h"
#include <mtty/aerospike.h>
#include <iostream>
#include <unistd.h>
#include "logging.h"

using namespace adservice::utility;
using namespace adservice::utility::time;


bool inDebugSession = false;
void * debugSession = nullptr;
MT::common::Aerospike aerospikeClient;

void testGetSync(const std::string& url,uint32_t timeoutMs){
        std::cout<<"get sync test:"<<std::endl;;
    auto httpClient = HttpClientProxy::getInstance();
    int64_t timeBegin = getCurrentTimeStampMs();
    const HttpResponse& response = httpClient->getSync(url,timeoutMs);
    std::cout<<"response status:"<<response.status()<<std::endl;
    std::cout<<"response body:"<<response.get_body()<<std::endl;
    std::cout<<"get async test:"<<std::endl;
    int64_t endTime = getCurrentTimeStampMs();
    std::cout<<"time collapsed:"<<(endTime-timeBegin)<<"ms"<<std::endl;
}

void testGet(const std::string& url,uint32_t timeoutMs){
    std::cout<<"get sync test:"<<std::endl;;
    auto httpClient = HttpClientProxy::getInstance();
    int64_t timeBegin = getCurrentTimeStampMs();
    future::PromisePtr operationPromise = std::make_shared<future::Promise>();
    operationPromise->after(httpClient->getAsync(url,timeoutMs,[timeBegin](int err,int status,const std::string& response){
        if(err!=0){
            LOG_ERROR<<"error ocurr,retcode:"<<err;
            LOG_ERROR<<"resposne status:"<<status;
            LOG_ERROR<<"response body:"<<response;
        }else{
            std::cout<<"response status:"<<status<<std::endl;
            std::cout<<"response body:"<<response<<std::endl;
        }
        int64_t endTime = getCurrentTimeStampMs();
        std::cout<<"time collapsed:"<<(endTime-timeBegin)<<"ms"<<std::endl;
    }));
    operationPromise->sync();
}

void testGetPressure(const std::string& url,uint32_t timeoutMs,int threadCnt){
    std::thread threads[10000];
    int* result = new int[threadCnt];
    memset(result,0,threadCnt*sizeof(int));
    future::PromisePtr operationPromise = std::make_shared<future::Promise>();
    for(int i=0;i<threadCnt;i++){
        threads[i] = std::thread([result,i,url,timeoutMs,operationPromise](){
            int64_t timeBegin = getCurrentTimeStampMs();
            auto httpClient = HttpClientProxy::getInstance();
            operationPromise->after(
            httpClient->getAsync(url,timeoutMs,[&result,i,timeBegin](int err,int status,const std::string& response){
                int64_t endTime = getCurrentTimeStampMs();
                result[i] = endTime-timeBegin;
                std::cout<<"finished:"<<result[i]<<",begin:"<<timeBegin<<",end:"<<endTime<<std::endl;
            }));
        });
    }
    operationPromise->sync();
    int64_t timeoutCnt = 0;
    for(int i=0;i<threadCnt;i++){
       threads[i].join();
       if(result[i]!=0){
           timeoutCnt+=result[i];
       }
    }
    std::cout<<"timeoutCnt:"<<timeoutCnt/threadCnt<<std::endl;
    delete[] result;
}

void testPostSync(const std::string& url,const std::string& contentType,const std::string& postData,uint32_t timeoutMs){
    std::cout<<"post sync test:"<<std::endl;
    auto httpClient = HttpClientProxy::getInstance();
    const HttpResponse& response = httpClient->postSync(url,postData,contentType,timeoutMs);
    std::cout<<"response status:"<<response.status()<<std::endl;
    std::cout<<"response body:"<<response.get_body()<<std::endl;
    std::cout<<"post async test:"<<std::endl;
}

void testPost(const std::string& url,const std::string& contentType,const std::string& postData,uint32_t timeoutMs){
    auto httpClient = HttpClientProxy::getInstance();
    future::PromisePtr operationPromise = std::make_shared<future::Promise>();
    operationPromise->after(httpClient->postAsync(url,postData,contentType,timeoutMs,[](int err,int status,const std::string& response){
        if(err!=0){
            LOG_ERROR<<"error ocurr"<<std::endl;
        }else{
            std::cout<<"response status:"<<status<<std::endl;
            std::cout<<"response body:"<<response<<std::endl;
        }
    }));
    operationPromise->sync();
}

void testPostPressure(const std::string& url,const std::string& contentType,const std::string& postData,uint32_t timeoutMs,int threadCnt){
    std::thread threads[10000];
    int* result = new int[threadCnt];
    memset(result,0,threadCnt*sizeof(int));
    future::PromisePtr operationPromise = std::make_shared<future::Promise>();
    for(int i=0;i<threadCnt;i++){
        threads[i] = std::thread([result,i,url,timeoutMs,contentType,postData,operationPromise](){
            int64_t timeBegin = getCurrentTimeStampMs();
            auto httpClient = HttpClientProxy::getInstance();
            operationPromise->after(
            httpClient->postAsync(url,postData,contentType,timeoutMs,[&result,i,timeBegin](int err,int status,const std::string& response){
                int64_t endTime = getCurrentTimeStampMs();
                result[i] = endTime-timeBegin;
                std::cout<<"finished:"<<result[i]<<",begin:"<<timeBegin<<",end:"<<endTime<<std::endl;
            }));
        });
    }
    operationPromise->sync();
    int64_t timeoutCnt = 0;
    for(int i=0;i<threadCnt;i++){
       threads[i].join();
       if(result[i]!=0){
           timeoutCnt+=result[i];
       }
    }
    std::cout<<"timeoutCnt:"<<timeoutCnt/threadCnt<<std::endl;
    delete[] result;
}

void testKeepAlive(const std::string& url,uint32_t timeoutMs,int32_t times){
    auto httpClient = HttpClientProxy::getInstance();
    httpClient->setVerbose(true);
    for(int i=0;i<times;i++){
        std::cout<<"iteration "<<i<<std::endl;
        int64_t timeBegin = getCurrentTimeStampMs();
        future::PromisePtr operationPromise = std::make_shared<future::Promise>();
        operationPromise->after(httpClient->getAsync(url,timeoutMs,[timeBegin](int err,int status,const std::string& response){
            if(err!=0){
                LOG_ERROR<<"error ocurr,retcode:"<<err;
                LOG_ERROR<<"resposne status:"<<status;
            }else{
                std::cout<<"response status:"<<status<<std::endl;
            }
            int64_t endTime = getCurrentTimeStampMs();
            std::cout<<"time collapsed:"<<(endTime-timeBegin)<<"ms"<<std::endl;
        }));
        operationPromise->sync();
    }
}

int main(int argc, char ** argv)
{
    HttpClientProxy::instance_ = std::make_shared<HttpClientProxy>();
    if(argc>2){
        std::string op=argv[1];
        if(op=="get" && argc == 4){
                testGet(argv[2],atoi(argv[3]));
                return 0;
        }else if(op=="getsync"&& argc == 4){
                testGetSync(argv[2],atoi(argv[3]));
                return 0;
        }else if(op=="getperform" && argc == 5){
                testGetPressure(argv[2],atoi(argv[3]),atoi(argv[4]));
                return 0;
        }else if(op=="postsync" && argc==6){
                testPostSync(argv[2],argv[3],argv[4],atoi(argv[5]));
                return 0;
        }else if (op == "post" && argc == 6){
                testPost(argv[2],argv[3],argv[4],atoi(argv[5]));
                return 0;
        }else if(op=="postperform" && argc==7){
                testPostPressure(argv[2],argv[3],argv[4],atoi(argv[5]),atoi(argv[6]));
                return 0;
        }else if(op=="testkeepalive" && argc==5){
                testKeepAlive(argv[2],atoi(argv[3]),atoi(argv[4]));
                return 0;
        }
    }
    std::cout<<"usage:"<<std::endl;
    std::cout<<"\t"<<argv[0]<<" get [url] [timeout]"<<std::endl;
    std::cout<<"\t"<<argv[0]<<" post [url] [content-type] [postdata] [timeout]"<<std::endl;
    std::cout<<"\t"<<argv[0]<<" testkeepalive [url] [timeout] [times]"<<std::endl;
    return 0;
}

