/***************************************
*	Description: Ipc module, used for process.
*	Author: wilson
*	Date: 2015/09/25
*
*	CopyRight: adirect Technology Co, Ltd
*
****************************************/
#include "AdIpc.h"
#include <sys/msg.h>
#include <errno.h>

AdIpc::AdIpc()
{
	m_iIpc = AD_FAILURE;
}
AdIpc::~AdIpc()
{
	Delete();
}

int AdIpc::Put(void* pMsg, UINT uiSize, bool bBlock)
{
	int ret;
	int iFlag = IPC_NOWAIT;
	if(bBlock)
		iFlag = 0;
	ret = msgsnd(m_iIpc, pMsg, uiSize, iFlag);
	//if((ret == AD_FAILURE) && ( errno == ENOMSG))
	//	return AD_SUCCESS;
	return ret;
}
	
int AdIpc::Get(void* pMsg, UINT uiSize, bool bBlock)
{
	int ret;
	int iFlag = IPC_NOWAIT;
	if(bBlock)
		iFlag = 0;
	ret = msgrcv(m_iIpc, pMsg, uiSize, 0, iFlag);
	if((ret == AD_FAILURE) && ( errno == EAGAIN))
		return AD_SUCCESS;
	return ret;
}

int AdIpc::Delete()
{
	if(m_iIpc == AD_FAILURE)
		return AD_SUCCESS;
	
	int ret = msgctl(m_iIpc, IPC_RMID, 0);
	if(ret == AD_SUCCESS)
	{
		m_iIpc = AD_FAILURE;
	}
	return AD_SUCCESS;
}

int AdIpc::Create(const char * pstPath, int iID)
{
	key_t iKey =  ftok(pstPath, iID); 
	m_iIpc = msgget(iKey, IPC_CREAT | 0777);
	if (AD_FAILURE == m_iIpc) 
	{
		AD_ERROR("Ipc create Error\n");
		return AD_FAILURE;
	}

	return AD_SUCCESS;
}


