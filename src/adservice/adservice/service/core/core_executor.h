//
// Created by guoze.lin on 16/2/13.
//

#ifndef ADCORE_CORE_EXECUTOR_H
#define ADCORE_CORE_EXECUTOR_H

#include "core_threadlocal_manager.h"
#include <deque>
#include <functional>
#include <mutex>
#include <unistd.h>
#include <vector>

namespace adservice {
namespace server {

    static const int EXECUTOR_MAX_TASK = 5000;
    static const int DEFAULT_CORE_NUM = 1;

    class Condition : boost::noncopyable {
    public:
        explicit Condition(std::mutex & mutex)
            : mutex_(mutex)
        {
            pthread_cond_init(&pcond_, NULL);
        }

        ~Condition()
        {
            pthread_cond_destroy(&pcond_);
        }

        void wait()
        {
            pthread_cond_wait(&pcond_, mutex_.native_handle());
        }

        void notify()
        {
            pthread_cond_signal(&pcond_);
        }

        void notifyAll()
        {
            pthread_cond_broadcast(&pcond_);
        }

    private:
        std::mutex & mutex_;
        pthread_cond_t pcond_;
    };

    typedef std::function<void()> Task;

    class ThreadPool {
    public:
        explicit ThreadPool(size_t threads, size_t queueSize);

        ~ThreadPool()
        {
            if (running_) {
                stop();
            }
        }

        void setThreadSize(size_t s)
        {
            threadSize_ = s;
        }

        void setQueueSize(size_t qs)
        {
            maxQueueSize_ = qs;
        }

        void start();

        void stop();

        void run(const Task & t);

        Task take();

        void setInitCallback(const Task && t)
        {
            threadInitCallback_ = t;
        }

        void runInitCallback();

        bool isRunning()
        {
            return running_;
        }

    private:
        std::vector<pthread_t> threads_;
        std::deque<Task> queue_;
        size_t threadSize_{ 10 };
        size_t maxQueueSize_{ 5000 };
        std::mutex mutex_;
        Task threadInitCallback_;
        bool running_{ false };
        Condition nofull_;
        Condition noempty_;
    };

    /**
     * Executor 用于维护线程池和任务队列
     * 对于计算密集任务,一个cpup核心上的多线程并不能增加性能,因此要充分发挥机器性能,线程数不应超出核心数
     * Executor也可以用于处理IO密集型任务
     */
    class Executor {
    public:
        /**
         * forCompute: 是否用于计算密集任务
         * threads: 如果forcompute=true,那么线程数等于机器核心数,否则线程池线程数为threads指定
         */
        Executor(const char * name, bool forCompute = true, int threads = 0, int taskQueueSize = 0)
            : coreNum(DEFAULT_CORE_NUM)
            , pureCompute(forCompute)
            , threadNum(threads == 0 ? 1 : threads)
            , threadpool(threadNum, taskQueueSize == 0 ? EXECUTOR_MAX_TASK : taskQueueSize)
        {
        }

        void start();

        void stop()
        {
            threadpool.stop();
        }
        void run(std::function<void()> && todo)
        {
            threadpool.run(todo);
        }
        void run(std::function<void()> & todo)
        {
            threadpool.run(todo);
        }

        int getThreadSeqId();

    private:
        void configureForCompute();

    private:
        int coreNum;
        int threadNum;
        bool pureCompute;
        long threadMappingTable[128];
        ThreadPool threadpool;
    };
}
}

#endif // ADCORE_CORE_EXECUTOR_H
