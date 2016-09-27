/***************************************
*	Description: shared library module, manage shared lib
*	Author: wilson
*	Date: 2015/12/24
*
*	CopyRight: Adirect Technology Co, Ltd
*
****************************************/

#ifndef  __AD_SHARED_LIB_H__
#define __AD_SHARED_LIB_H__

#include <map>
#include <vector>
#include <iostream>
#include <string>
#include <dlfcn.h>

#include "AdGlobal.h"
#include "AdBuff.h"

using namespace std;

template<typename Type, typename Request>
class AdSharedLib
{
public:
	AdSharedLib()
	{
		m_bPingPong = false;
	};
	
	virtual ~AdSharedLib()
	{
		Close();
	};

	void Clear(bool bDel = false)
	{
		typename map<Type, Request >::iterator iterR;
		map<Type, Request>& mRequest = m_ctRequests.Get(m_bPingPong);

		if(mRequest.empty())
			return;
		if(bDel)
		{
			for(iterR = mRequest.begin(); iterR != mRequest.end(); iterR ++)
			{
				delete iterR->second;
			}
		}
		mRequest.clear();
	};
	
	void Close(bool bDel = false)
	{
		Clear(bDel);
		Switch();
		Clear(bDel);
		
		typename map<Type, void * >::iterator  iterV;
		for(iterV = m_mShareLib.begin(); iterV != m_mShareLib.end(); iterV ++)
		{
			dlclose(iterV->second);
		}
		m_mShareLib.clear();
	};
	
	typedef int ( * INITFUNC)(Request &);
		
	int Open(const Type  & Id, const string & sShareLibFile, bool bPingPong = false)
	{
		void *handle;
		Request ctReq;
		INITFUNC  func;
	
		handle = dlopen(sShareLibFile.c_str(), RTLD_NOW);
		if (handle == NULL)
		{
			AD_ERROR("Failed to open libaray %s error:%s\n", sShareLibFile.c_str(), dlerror());
			return AD_FAILURE;
		}
		
		func = (INITFUNC)dlsym(handle, "SharedLibInit");
		if(func == NULL)
		{
			AD_ERROR("Failed to Find sym SharedLibInit error:%s\n", dlerror());
			return AD_FAILURE;
		}
	
		
		if(func(ctReq) != AD_SUCCESS)
		{
			dlclose(handle);
			return AD_FAILURE;
		}
	

		m_ctRequests.Get()[Id] = ctReq;
		m_mShareLib[Id] = handle;

		//ping pong mode
		m_bPingPong = bPingPong;
		if(m_bPingPong)
		{
			if(func(ctReq) != AD_SUCCESS)
			{
				dlclose(handle);
				return AD_FAILURE;
			}

			m_ctRequests.GetBack()[Id] = ctReq;
		}
	
		return AD_SUCCESS;
	};

	int  Get(const Type & id, Request & req)
	{
		map<Type, Request> mRequest = m_ctRequests.Get();
		typename map<Type, Request>::iterator	iter;
		iter = mRequest.find(id);
		if(iter == mRequest.end())
			return AD_FAILURE;
		req = iter->second;
		return AD_SUCCESS;
	};

	int  Switch()
	{
		m_ctRequests.Switch();
		return AD_SUCCESS;
	};
private:
	AdPingPongBuffer<map<Type, Request> > m_ctRequests;
	map<Type, void * > m_mShareLib;
	bool m_bPingPong;
};

#endif

