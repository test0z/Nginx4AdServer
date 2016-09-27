//
// Created by guoze.lin on 16/2/13.
//

#ifndef ADCORE_CORE_EXECUTOR_H
#define ADCORE_CORE_EXECUTOR_H

#include <unistd.h>
#include "muduo/base/ThreadPool.h"
#include "core_threadlocal_manager.h"

namespace adservice{
    namespace server{

        using namespace muduo;

        static const int EXECUTOR_MAX_TASK = 5000;
        static const int DEFAULT_CORE_NUM = 1;
        /**
         * Executor 用于维护线程池和任务队列
         * 对于计算密集任务,一个cpup核心上的多线程并不能增加性能,因此要充分发挥机器性能,线程数不应超出核心数
         * Executor也可以用于处理IO密集型任务
         */
        class Executor{
        public:
            /**
             * forCompute: 是否用于计算密集任务
             * threads: 如果forcompute=true,那么线程数等于机器核心数,否则线程池线程数为threads指定
             */
            Executor(const char* name,bool forCompute = true,int threads = 0,int taskQueueSize = 0):threadpool(name){
                coreNum = DEFAULT_CORE_NUM;
                threadNum = threads == 0 ? 1 : threads;
                pureCompute = forCompute;
                threadpool.setMaxQueueSize(taskQueueSize==0?EXECUTOR_MAX_TASK:taskQueueSize);
            }

            void start();

            void stop(){
                threadpool.stop();
            }
            void run(std::function<void()>&& todo){
                threadpool.run(todo);
            }
            void run(std::function<void()>& todo){
                threadpool.run(todo);
            }
            int pendingTasks(){
                return threadpool.queueSize();
            }
            int  getThreadSeqId();
        private:
            void configureForCompute();

        private:
            int coreNum;
            int threadNum;
            bool pureCompute;
            ThreadPool threadpool;
            long threadMappingTable[128];
        };

    }
}

#endif //ADCORE_CORE_EXECUTOR_H
