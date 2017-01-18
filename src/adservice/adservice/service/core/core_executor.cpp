//
// Created by guoze.lin on 16/2/14.
//

#include "core_executor.h"
#include "common/spinlock.h"
#include "logging.h"
#include <algorithm>
#include <sched.h>

namespace adservice {
namespace server {

    void threadpool_thread_run(void * data)
    {
        try {
            ThreadPool * threadPool = (ThreadPool *)data;
            threadPool->runInitCallback();
            while (threadPool->isRunning()) {
                Task task(threadPool->take());
                if (task) {
                    task();
                }
            }
        } catch (std::exception & e) {
            LOG_ERROR << "threadpool_thread_run encounter with exception,e:" << e.what();
        }
    }

    ThreadPool::ThreadPool(size_t threads, size_t queueSize)
        : mutex_()
        , threadSize_(threads)
        , maxQueueSize_(queueSize)
        , nofull_(mutex_)
        , noempty_(mutex_)
    {
    }

    void ThreadPool::start()
    {
        running_ = true;
        for (uint32_t i = 0; i < threadSize_; i++) {
            pthread_t threadId;
            if (pthread_create(&threadId, NULL, threadpool_thread_run, this)) {
                LOG_ERROR << "ThreadPool start pthread_create failed";
            } else {
                threads_.push_back(threadId);
            }
        }
    }

    void ThreadPool::stop()
    {
        running_ = false;
        noempty_.notifyAll();
        for (pthread_t t : threads_) {
            pthread_join(t, nullptr);
        }
        threads_.clear();
    }

    void ThreadPool::runInitCallback()
    {
        if (threadInitCallback_) {
            threadInitCallback_();
        }
    }

    Task ThreadPool::take()
    {
        mutex_.lock();
        while (queue_.empty() && running_) {
            noempty_.wait();
        }
        Task task;
        if (!queue_.empty()) {
            task = queue_.front();
            queue_.pop_front();
            if (maxQueueSize_ > 0) {
                nofull_.notify();
            }
        }
        mutex_.unlock();
        return task;
    }

    void ThreadPool::run(const Task & t)
    {
        mutex_.lock();
        while (maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_) {
            nofull_.wait();
        }
        queue_.push_back(t);
        noempty_.notify();
        mutex_.unlock();
    }

    struct RunInCore {
        /**
         * tc: 所有核心数
         * threadTable:需要填充的线程表
         * threadNum:所有线程数
         */
        RunInCore(int tc, long * threadTable, int threadNum)
        {
            runCore = 0;
            totalCore = tc;
            totalThread = threadNum;
            threadMappingTable = threadTable;
            SPIN_INIT(this);
        }
        void operator()()
        {
            int myCore = 0;
            SPIN_LOCK(this);
            myCore = runCore++;
            SPIN_UNLOCK(this);
            pthread_t me = pthread_self();
            threadMappingTable[myCore] = me;
            myCore %= totalCore;
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET(myCore, &cpuset);
            pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
            if (runCore == totalThread) {
                std::sort<long *>(threadMappingTable, threadMappingTable + totalThread);
            }
        }
        ~RunInCore()
        {
            SPIN_DESTROY(this);
        }
        volatile int runCore;
        int totalCore;
        int totalThread;
        spinlock lock;
        long * threadMappingTable;
    };

    struct DefaultThreadInitializer {
        void operator()()
        {
        }
    };

    void Executor::start()
    {
        if (pureCompute) {
            configureForCompute();
            threadpool.setInitCallback(std::bind(RunInCore(coreNum, threadMappingTable, threadNum)));
        } else {
            threadpool.setInitCallback(std::bind(DefaultThreadInitializer()));
        }
        threadpool.setThreadSize(threadNum);
        threadpool.start();
    }

    void Executor::configureForCompute()
    {
        long nprocessors = sysconf(_SC_NPROCESSORS_ONLN);
        if (nprocessors <= 0) {
            coreNum = DEFAULT_CORE_NUM;
        } else {
            coreNum = nprocessors;
        }
        LOG_INFO << "core executor core num:" << coreNum;
    }

    int Executor::getThreadSeqId()
    {
        long key = (long)pthread_self();
        int l = 0, h = threadNum - 1;
        while (l <= h) {
            int mid = l + ((h - l) >> 1);
            if (key <= threadMappingTable[mid])
                h = mid - 1;
            else
                l = mid + 1;
        }
        assert(l >= 0 && l < threadNum && threadMappingTable[l] == key);
        return l;
    }
}
}
