//
// Created by guoze.lin on 16/2/14.
//

#include <sched.h>
#include <algorithm>
#include "core_executor.h"
#include "common/spinlock.h"
#include "logging.h"

namespace adservice{
    namespace server{

        using namespace Logging;

        struct RunInCore{
            /**
             * tc: 所有核心数
             * threadTable:需要填充的线程表
             * threadNum:所有线程数
             */
            RunInCore(int tc,long* threadTable,int threadNum){
                runCore = 0;
                totalCore = tc;
                totalThread = threadNum;
                threadMappingTable = threadTable;
                SPIN_INIT(this);
            }
            void operator()(){
                int myCore = 0;
                SPIN_LOCK(this);
                myCore = runCore++;
                SPIN_UNLOCK(this);
                pthread_t me = pthread_self();
                threadMappingTable[myCore] = me;
                myCore%=totalCore;
                cpu_set_t cpuset;
                CPU_ZERO(&cpuset);
                CPU_SET(myCore,&cpuset);
                pthread_setaffinity_np(pthread_self(),sizeof(cpu_set_t),&cpuset);
                if(runCore==totalThread){
                    std::sort<long*>(threadMappingTable,threadMappingTable+totalThread);
                }
            }
            ~RunInCore(){
                SPIN_DESTROY(this);
            }
            volatile int runCore;
            int totalCore;
            int totalThread;
            spinlock lock;
            long* threadMappingTable;
        };

        struct DefaultThreadInitializer{
            void operator()(){
            }
        };

        void Executor::start(){
            if(pureCompute){
                configureForCompute();
                threadpool.setThreadInitCallback(std::bind(RunInCore(coreNum,threadMappingTable,threadNum)));
            }else{
                threadpool.setThreadInitCallback(std::bind(DefaultThreadInitializer()));
            }
            threadpool.start(threadNum);
        }

        void Executor::configureForCompute() {
            long nprocessors = sysconf(_SC_NPROCESSORS_ONLN);
            if(nprocessors<=0){
                coreNum = DEFAULT_CORE_NUM;
            }else{
                coreNum = nprocessors;
            }
            LOG_INFO<<"core executor core num:"<<coreNum;
        }

        int Executor::getThreadSeqId(){
            long key = (long)pthread_self();
            int l=0,h=threadNum-1;
            while(l<=h){
                int mid = l+((h-l)>>1);
                if(key<=threadMappingTable[mid])
                    h = mid-1;
                else
                    l = mid+1;
            }
            assert(l>=0&&l<threadNum&&threadMappingTable[l]==key);
            return l;
        }

    }
}