/***************************************
*	Description: Ipc module, used for process.
*	Author: wilson
*	Date: 2015/09/25
*
*	CopyRight: adirect Technology Co, Ltd
*
****************************************/

#ifndef   __AD_IPC_H__
#define __AD_IPC_H__

#include "AdGlobal.h"
#include <sys/ipc.h>
#include <sys/types.h>
#include <unistd.h>


class AdIpc
{
public:
	AdIpc();
	virtual ~AdIpc();
	int Get(void* pMsg, UINT uiSize, bool bBlock);
	int Put(void* pMsg, UINT uiSize, bool bBlock);
	int Delete();
	int Create(const char * pstPath, int iID);

private:
	int m_iIpc;

};

#endif

