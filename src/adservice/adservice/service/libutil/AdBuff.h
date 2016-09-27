/***************************************
*	Description:  buff module.
*	Author: wilson
*	Date: 2015/11/18
*
*	CopyRight: Adirect Technology Co, Ltd
*
****************************************/

#ifndef	_AD_BUFF_H
#define	_AD_BUFF_H

#include <vector>
#include <map>
#include <string>
#include <iostream>
#include "AdGlobal.h"
#include "AdLock.h"

using namespace std;

//ring buffer base, loop buff;
template<typename Request>
class AdRingBufferBase
{
public:
	AdRingBufferBase(int iSize = AD_FAILURE)
	{
		init(iSize);
	};
	
	AdRingBufferBase(AdRingBufferBase & other)
        {
		*this = other;
	};

	AdRingBufferBase & operator = (const AdRingBufferBase & other)
	{
		m_vRing = other.m_vRing;
		m_iSize = other.m_iSize;
		other.m_iSize = 0;
		m_iReadPosition = other.m_iReadPosition;
		other.m_iReadPosition = 0;
		m_iWritePosition = other.m_iWritePosition;
		other.m_iWritePosition = 0;
		return *this;
	};

	void init(int iSize)
	{
		if(iSize > 0)
			m_vRing.resize(iSize);
		m_iSize = iSize;
		m_iReadPosition = 0;
		m_iWritePosition = 0;
	}

	inline void __push(const Request & request)
	{
		m_vRing[m_iWritePosition] = request;
	        m_iWritePosition = (m_iWritePosition + 1) % m_iSize;
	};
	
	inline bool  __full()
	{
		int fullReadPosition = (m_iWritePosition + 1) % m_iSize;
	        return m_iReadPosition == fullReadPosition;
	};

	inline bool  __empty()
	{
	        return m_iReadPosition == m_iWritePosition;
	};

	inline  void __pop( Request & request)
	{
		request = m_vRing[m_iReadPosition];
	        m_iReadPosition = (m_iReadPosition + 1) % m_iSize;
	};
	inline  UINT size()
	{
		if(m_iReadPosition == m_iWritePosition)
			return 0;
		else if(m_iReadPosition > m_iWritePosition)
		{
			return m_iSize - m_iReadPosition  + m_iWritePosition;
		}
		else
			return m_iWritePosition  - m_iReadPosition;
	}
	
private:
	std::vector<Request> m_vRing;
	int m_iSize;
	int m_iReadPosition;
	int m_iWritePosition;
};

// RING BUFFER MULTIPLE READER MULTIPLE WRITERS                             
template<typename Request>
class AdRingBufferMRMW : public AdRingBufferBase<Request> 
{
public:
	AdRingBufferMRMW(int size) : AdRingBufferBase<Request>(size)
	{
	}

	void push(const Request & request, int iMs = 0)
	{
		m_cWriteLock.lock();
		while(true)
		{  
	        	if (AdRingBufferBase<Request>::__full())
	        		m_cWriteLock.wait(iMs);
	            	else 
				break;
	        }
	        this->__push(request);
	        m_cWriteLock.unlock();
		m_cReadLock.wake();
	};

	bool tryPush(const Request & request)
	{ 
	       m_cWriteLock.lock();
	       if (AdRingBufferBase<Request>::__full())
	        {
	        	m_cWriteLock.unlock();
			return false;
	        }
	       this->__push(request);
	        m_cWriteLock.unlock();
		m_cReadLock.wake();
	        return true;
	}


	Request pop(int iMs = 0)
	{
		Request result;
		m_cReadLock.lock();
		while(true)
		{
	        	if (AdRingBufferBase<Request>::__empty())
	        		m_cReadLock.wait(iMs);
	            	else 
				break;
	        }
		pop(result);
	        m_cReadLock.unlock();
		m_cWriteLock.wake();

	        return result;
	};


	bool tryPop(Request & result)
	{
		m_cReadLock.lock();
	        if (AdRingBufferBase<Request>::__empty())
	        {
	        	m_cReadLock.unlock();
			return false;
	        }
		this->__pop(result);
	        m_cReadLock.unlock();
		m_cWriteLock.wake();
	        return true;
	};

private:
	LockCond  m_cReadLock;
	LockCond  m_cWriteLock;
};


// RING BUFFER SINGLE READER MULTIPLE WRITERS  
template<typename Request>
class AdRingBufferSRMW : public AdRingBufferBase<Request> 
{
public:
	AdRingBufferSRMW(int size) : AdRingBufferBase<Request>(size)
	{
	}

	void push(const Request & request, int iMs = 0)
	{
		m_cWriteLock.lock();
		while(true)
		{  
	        	if (AdRingBufferBase<Request>::__full())
	        		m_cWriteLock.wait(iMs);
	            	else 
				break;
	        }

	        this->__push(request);
	        m_cWriteLock.unlock();
		m_cReadLock.wake();
	};

	bool tryPush(const Request & request)
	{ 
	       m_cWriteLock.lock();
	       if (AdRingBufferBase<Request>::__full())
	        {
	        	m_cWriteLock.unlock();
			return false;
	        }
	       this->__push(request);
	        m_cWriteLock.unlock();
		m_cReadLock.wake();
	        return true;
	}


	Request pop(int iMs = 0)
	{
		Request result;
		while(true)
		{
	        	if (AdRingBufferBase<Request>::__empty())
	        		popWait(iMs);
	            	else 
				break;
	        }
		this->__pop(result);
		m_cWriteLock.wake();

	        return result;
	};

	void popWait(int iMs = 0)
	{
		m_cReadLock.lock();
		m_cReadLock.wait(iMs);
		m_cReadLock.unlock();
	};


	bool tryPop(Request & result)
	{
	        if (AdRingBufferBase<Request>::__empty())
	        {	    
			return false;
	        }
		this->__pop(result);
		m_cWriteLock.wake();
	        return true;
	};

private:
	LockCond  m_cReadLock;
	LockCond  m_cWriteLock;
};

// RING BUFFER MULTIPLE READER  SINGLE WRITERS  
template<typename Request>
class AdRingBufferMRSW : public AdRingBufferBase<Request> 
{
public:
	AdRingBufferMRSW(int size) : AdRingBufferBase<Request>(size)
	{
	}
	
	void pushWait(int iMs = 0)
	{
		m_cWriteLock.lock();
		m_cWriteLock.wait(iMs);
		m_cWriteLock.unlock();
	};
	
	void push(const Request & request, int iMs = 0)
	{
		while(true)
		{
	        	if (AdRingBufferBase<Request>::__full())
	        		pushWait(iMs);
	            	else 
				break;
	        }
	       this->__push(request);
		m_cReadLock.wake();
	};

	bool tryPush(const Request & request)
	{
	       if (AdRingBufferBase<Request>::__full())
	        {
			return false;
	        }
		 this->__push(request);
		m_cReadLock.wake();
	        return true;
	}


	Request pop(int iMs = 0)
	{
		Request result;
		m_cReadLock.lock();
		while(true)
		{
	        	if (AdRingBufferBase<Request>::__empty())
	        		m_cReadLock.wait(iMs);
	            	else 
				break;
	        }
		pop(result);
	        m_cReadLock.unlock();
		m_cWriteLock.wake();

	        return result;
	};


	bool tryPop(Request & result)
	{
		m_cReadLock.lock();
	 
	        if (AdRingBufferBase<Request>::__empty())
	        {
	        	m_cReadLock.unlock();
			return false;
	        }
		this->__pop(result);
	        m_cReadLock.unlock();
		m_cWriteLock.wake();
	        return true;
	};

private:
	LockCond  m_cReadLock;
	LockCond  m_cWriteLock;
};


// RING BUFFER SINGLE READER  SINGLE WRITERS  
template<typename Request>
class AdRingBufferSRSW : public AdRingBufferBase<Request> 
{
public:
	AdRingBufferSRSW(int size) : AdRingBufferBase<Request>(size)
	{
		m_uiReadCount = 0;
		m_uiWriteCount = 0;
	}
	
	void pushWait(int iMs = 0)
	{
		m_cWriteLock.lock();
		m_cWriteLock.wait(iMs);
		m_cWriteLock.unlock();
	};

	void popWait(int iMs = 0)
	{
		m_cReadLock.lock();
		m_cReadLock.wait(iMs);
		m_cReadLock.unlock();
	};
	
	void push(const Request & request, int iMs = 0)
	{
		while(true)
		{
	        	if (AdRingBufferBase<Request>::__full())
	        		pushWait(iMs);
	            	else 
				break;
	        }
	       this->__push(request);
		m_uiWriteCount ++; 
		m_cReadLock.wake();
	};

	bool tryPush(const Request & request)
	{
	       if (AdRingBufferBase<Request>::__full())
	        {
			return false;
	        }
		 this->__push(request);
		 m_uiWriteCount ++; 
		m_cReadLock.wake();
	        return true;
	}


	Request pop(int iMs = 0)
	{
		Request result;
		while(true)
		{
	        	if (AdRingBufferBase<Request>::__empty())
	        		popWait(iMs);
	            	else 
				break;
	        }
		this->__pop(result);
		m_uiReadCount ++; 
		m_cWriteLock.wake();

	        return result;
	};


	bool tryPop(Request & result)
	{
	
	        if (AdRingBufferBase<Request>::__empty())
	        {
			return false;
	        }
		this->__pop(result);
		m_uiReadCount ++; 
		m_cWriteLock.wake();
	        return true;
	};

private:
	LockCond  m_cReadLock;
	LockCond  m_cWriteLock;
	UINT  m_uiReadCount;
	UINT  m_uiWriteCount;
};

//LOOP READ, ADD DEL,  MULTIPLE READER MULTIPLE WRITERS 
class AdLRBuffer
{
public:
	AdLRBuffer(){};
	~AdLRBuffer(){};
	void push(void *  pData)
	{
		std::map<void *, UINT>::iterator iter;
		m_cLock.lockWrite();
		iter = m_mBuffer.find(pData);
		if(iter == m_mBuffer.end())
		{
			m_mBuffer[pData] = m_vBuffer.size();
			m_vBuffer.push_back(pData);
		}
		m_cLock.unlock();
	};
	void del(void *  pData)
	{	
		std::map<void *, UINT>::iterator iter;
		m_cLock.lockWrite();
		iter = m_mBuffer.find(pData);
		if(iter != m_mBuffer.end())
		{
			m_mBuffer.erase(iter);
			m_vBuffer.clear();
			for(iter = m_mBuffer.begin(); iter != m_mBuffer.end(); iter ++)
			{
				iter->second = m_vBuffer.size();
				m_vBuffer.push_back(iter->first);
			}
		}
		m_cLock.unlock();
	};
	
	void * find( UINT id)
	{
		void * pResult = NULL;
		std::map<void *, UINT>::iterator iter;
		m_cLock.lockRead();
		if(!m_vBuffer.empty())
		{
			id = id % (UINT)m_vBuffer.size();
			pResult = m_vBuffer[id];
		}
		m_cLock.unlock();
		return pResult;
	}; 

	bool check(void * pData)
	{
		std::map<void * , UINT>::iterator iter;
		m_cLock.lockRead();
		iter = m_mBuffer.find(pData);
		if(iter != m_mBuffer.end())
		{
			m_cLock.unlock();
			return true;
		}
		m_cLock.unlock();
		return false;
	};
	
	int size()
	{
		int num;
		m_cLock.lockRead();
		num = m_mBuffer.size();
		m_cLock.unlock();
		return num;
	}
private:
	std::map<void *, UINT> m_mBuffer;
	std::vector<void *> m_vBuffer;
	LockReadWrite  m_cLock;
};

//Ping pong buffer;
template<typename Request>
class AdPingPongBuffer
{
public:
	AdPingPongBuffer()
	{
		m_bFirst =false;
	};
	
	~AdPingPongBuffer()
        {
	};

	Request & Get(bool bBack = false)
	{
		if(bBack)
			return GetBack();
		
		if(m_bFirst)
			return m_ctRequestA;
		return m_ctRequestB;
	};

	Request & GetBack()
	{
		if(m_bFirst)
			return m_ctRequestB;
		return m_ctRequestA;
	};

	void Switch()
	{
		if(m_bFirst)
			m_bFirst = false;
		else
			m_bFirst = true;
	};
private:
	Request  m_ctRequestA;
	Request  m_ctRequestB;
	bool m_bFirst;
};


#endif

