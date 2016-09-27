/***************************************
*	Description: Os Systerm interface, Thread.
*	Author: wilson
*	Date: 2015/11/18
*
*	CopyRight: Adirect Technology Co, Ltd
*
****************************************/

#ifndef AD_LOCK_H
#define  AD_LOCK_H
#include <pthread.h>
#include <sys/time.h>
#include "AdGlobal.h"


//condition lock
class LockCond
{
public:
	LockCond()
	{
		int ret =   pthread_cond_init(&m_cCond,   NULL);
		if(ret != AD_SUCCESS)
		{
			AD_ERROR("Cond Init Faile %d\n", ret);
		}
		ret =   pthread_mutex_init(&m_cMutex,   NULL);
		if(ret != AD_SUCCESS)
		{
			AD_ERROR("Mutex Init Faile %d\n", ret);
		}
	}
	virtual ~LockCond()
	{
		pthread_cond_destroy(&m_cCond);
		pthread_mutex_destroy(&m_cMutex);
	};

	inline int wait(int iMs = 0)
	{	
		struct timeval now;
		 struct timespec outTime;
		if(iMs > 0)
		{	
			gettimeofday(&now, NULL);
			outTime.tv_sec = now.tv_sec;
    			outTime.tv_nsec = now.tv_usec + iMs * 1000;
			if(outTime.tv_nsec > 999999)
			{
				outTime.tv_sec += outTime.tv_nsec/1000000;
				outTime.tv_nsec -= outTime.tv_nsec%1000000;
			}
    			return pthread_cond_timedwait(&m_cCond, &m_cMutex, &outTime);
		}
		return pthread_cond_wait(&m_cCond, &m_cMutex);
	};

	inline int wake()
	{
		return pthread_cond_signal(&m_cCond);
	};

	inline int lock()
	{
		return pthread_mutex_lock(&m_cMutex);
	};

	inline int unlock()
	{
		return pthread_mutex_unlock(&m_cMutex);
	};

private:
	pthread_cond_t   m_cCond;
	pthread_mutex_t   m_cMutex;
};

//read write lock
class LockReadWrite
{
public:
	LockReadWrite()
	{
		int ret =   pthread_rwlock_init(&m_cRWLock,   NULL);
		if(ret != AD_SUCCESS)
		{
			AD_ERROR("Read Write Init Faile %d\n", ret);
		}
	}
	virtual ~LockReadWrite()
	{
		pthread_rwlock_destroy(&m_cRWLock);
	};

	inline int lockRead()
	{
		return pthread_rwlock_rdlock(&m_cRWLock);
	};

	inline int lockWrite()
	{
		return pthread_rwlock_wrlock(&m_cRWLock);
	};

	inline int unlock()
	{
		return pthread_rwlock_unlock(&m_cRWLock);
	};

private:
	pthread_rwlock_t   m_cRWLock;
};

//condition lock
class LockMutex
{
public:
	LockMutex()
	{
		int ret =   pthread_mutex_init(&m_cMutex,   NULL);
		if(ret != AD_SUCCESS)
		{
			AD_ERROR("Mutex Init Faile %d\n", ret);
		}
	}
	virtual ~LockMutex()
	{
		pthread_mutex_destroy(&m_cMutex);
	};

	inline int lock()
	{
		return pthread_mutex_lock(&m_cMutex);
	};

	inline int unlock()
	{
		return pthread_mutex_unlock(&m_cMutex);
	};

private:
	pthread_mutex_t   m_cMutex;
};

#endif
