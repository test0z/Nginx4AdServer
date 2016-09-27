/********************************************************/
/*	Geo Polymerization Technology Co, Ltd		*/
/*	Project:	GtLib-1.1.0			*/
/*	Author:		gong_libin			*/
/*	Date:		2013_09_11			*/
/*	File:		AdAcbm.h			*/
/********************************************************/
#ifndef	_AD_ACBM_H
#define	_AD_ACBM_H
#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>
#include "AdGlobal.h"
#include "AdUtil.h"
#include <ctype.h>



using namespace std;
#define	AD_ACBM				256


typedef struct tagAdAcbmNode
{
	int m_iLabel;
	int m_iDepth;

	UCHAR m_ucVal;
	int m_iGSShift;
	int m_iBCShift;

	int m_iChild;
	UCHAR m_ucChild;
	struct tagAdAcbmNode* m_pstParent;
	struct tagAdAcbmNode* m_pstChild[AD_ACBM];
}ADACBMNODE_S;

class AdAcbmTree
{
public:
	AdAcbmTree()
	{
		m_iPtnCount = 0;
		m_iMaxDepth = 0;
		m_iMinPtnSize = AD_BYTE512;
		m_pstRoot = NULL;
	};
	
	~AdAcbmTree()
	{
		clear();
	};
	void clear()
	{
		int i;
		m_iPtnCount = 0;
		m_iMaxDepth = 0;
		m_iMinPtnSize = AD_BYTE512;
		for(i = 0; i < AD_ACBM; i ++)
		{
			m_iBCShift[i] = 0;
		}
		if(m_pstRoot != 0)
		{
			AdAcbmTreeClean(m_pstRoot);
			free(m_pstRoot);
			m_pstRoot = NULL;
		}
		m_vStr.clear();
		m_mData.clear();
	};

	void AdAcbmTreeClean(ADACBMNODE_S* pstRoot)
	{
		int iCount = 0;

		for (iCount = 0; iCount < AD_ACBM; iCount ++) 
		{
			if ((iCount >= 'A') && (iCount <= 'Z')) continue;
			if (NULL != pstRoot->m_pstChild[iCount]) 
			{
				AdAcbmTreeClean(pstRoot->m_pstChild[iCount]);
				free(pstRoot->m_pstChild[iCount]);
				pstRoot->m_pstChild[iCount] = NULL;
			}
		}

		return;
	};
	
	int m_iPtnCount;
	int m_iMaxDepth;
	int m_iMinPtnSize;
	int m_iBCShift[AD_ACBM];
	ADACBMNODE_S* m_pstRoot;
	vector<string> m_vStr;
	map<int, list<void *> > m_mData;
};

class AdAcbm
{
public:
	AdAcbm();
	virtual ~AdAcbm();
	int AdAcbmTreeAdd(const string & sStr, void * pData);
	void AdAcbmTreeBuild();
	int AdAcbmSearch(const string & sStr, map<void*, list<string> > & mResult);
	void AdAcbmDisplayMatch(int iNum, int base);

protected:
	int AdAcbmSetGSShift2(string & sStr1, string & sStr2);
	int AdAcbmGSShiftInitCore(ADACBMNODE_S* pstRoot, int iShift);
	int AdAcbmSetGSShift1(string & sStr, int iDepth, int iShift);
	void AdAcbmGSShift();
	void AdAcbmBCShift();
	void AdAcbmGSShiftInit();

private:
	AdAcbmTree  m_ctTree;
	void AdAcbmClean();
};

#endif /* _ADACBM_H */
