//
// Created by guoze.lin on 16/2/15.
//

#include "core_threadlocal_manager.h"
#include "logging.h"


namespace adservice{
    namespace server{

        using namespace Logging;

        ThreadLocalManager& ThreadLocalManager::getInstance(){
            static ThreadLocalManager instance;
            return instance;
        }

        ThreadLocalManager::~ThreadLocalManager(){
            LOG_INFO<<"ThreadLocalManager gone";
            destroy();
        }

        void ThreadLocalManager::destroy() {
            static bool destroyOnce;
            if(!destroyOnce){
                destroyOnce = true;
                typedef typename std::map<pthread_t,pthread_key_t>::iterator Iter;
                for(Iter p  =threadLocalEntry.begin();p!=threadLocalEntry.end();p++){
                    pthread_key_delete(p->second);
                }
                LOG_INFO<<"ThreadLocalManager destroyed";
            }
        }

        void ThreadLocalManager::registerEntry(const pthread_t& t,pthread_key_t& k,void* dataPtr,void(*destructor)(void*)){
            if(k==0) {
                pthread_key_create(&k, destructor);//destructor?
                spinlock_lock(&slock);
                threadLocalEntry[t] = k;
                spinlock_unlock(&slock);
            }
            pthread_setspecific(k,dataPtr);
        }

        void* ThreadLocalManager::get(const pthread_t& t){
            pthread_key_t key = 0;
            spinlock_lock(&slock);
            if(threadLocalEntry.find(t)!=threadLocalEntry.end())
                key = threadLocalEntry[t];
            spinlock_unlock(&slock);
            if(key == 0)
                return NULL;
            void* dataPtr = pthread_getspecific(key);
            return dataPtr;
        }

        void ThreadLocalManager::put(const pthread_t& t,void* dataPtr,void(*destructor)(void*)){
            pthread_key_t key = 0;
            spinlock_lock(&slock);
            if(threadLocalEntry.find(t)!=threadLocalEntry.end())
                key = threadLocalEntry[t];
            spinlock_unlock(&slock);
            if(key ==0) {
                registerEntry(t, key, dataPtr,destructor);
            }else {
                pthread_setspecific(t, dataPtr);
            }
        }

    }
}