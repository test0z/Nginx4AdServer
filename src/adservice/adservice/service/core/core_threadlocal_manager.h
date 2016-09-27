//
// Created by guoze.lin on 16/2/15.
//

#ifndef ADCORE_CORE_THREADLOCAL_MANAGER_H
#define ADCORE_CORE_THREADLOCAL_MANAGER_H

#include <map>
#include <unistd.h>
#include "common/functions.h"
#include "common/spinlock.h"

namespace adservice{
    namespace server{

        using namespace std;

        struct ThreadLocalManager{
        public:
            static ThreadLocalManager& getInstance();
        private:
            ThreadLocalManager(){
                slock.lock = 0;
            }
        public:
            ThreadLocalManager(ThreadLocalManager& o) = delete;
            ~ThreadLocalManager();
            void registerEntry(const pthread_t& t,pthread_key_t& k,void* dataPtr,void(*destructor)(void*)=NULL);
            void* get(const pthread_t& t);
            void put(const pthread_t& t,void* dataPtr,void(*destructor)(void*)=NULL);
            void destroy();
        public:
            map<pthread_t,pthread_key_t> threadLocalEntry; //should use tbb concurrent map to improve effeciency
            struct spinlock slock;
        };

    }
}

#endif //ADCORE_CORE_THREADLOCAL_MANAGER_H
