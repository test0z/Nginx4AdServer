/********************************************************/
/*	Geo Polymerization Technology Co, Ltd		*/
/*	Project:	AdLib-1.1.0			*/
/*	Author:		gong_libin			*/
/*	Date:		2013_09_11			*/
/*	File:		AdAcbm.cpp			*/
/********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "AdAcbm.h"
//#include "common/functions.h"

AdAcbm::AdAcbm()
{
}

AdAcbm::~AdAcbm()
{
	m_ctTree.clear();
}

void AdAcbm::AdAcbmTreeBuild()
{
	AdAcbmBCShift();
	AdAcbmGSShiftInit();
	AdAcbmGSShift();

	return;
}

//******************BS shfit**********************************/

void AdAcbm::AdAcbmBCShift()
{
	int iOne = 0;
	int iTwo = 0;
	UCHAR ucVal = '\0';

	for (iOne = 0; iOne < AD_ACBM; iOne ++) 
	{
		m_ctTree.m_iBCShift[iOne] = m_ctTree.m_iMinPtnSize;
	}


	for (iOne = m_ctTree.m_iMinPtnSize - 1; iOne > 0; iOne --) 
	{
		for (iTwo = 0; iTwo < m_ctTree.m_iPtnCount; iTwo ++) 
		{
			ucVal = m_ctTree.m_vStr[iTwo][iOne];
			ucVal = tolower(ucVal);
			m_ctTree.m_iBCShift[ucVal] = iOne;
			if ((ucVal > 'a') && (ucVal < 'z')) 
			{
				m_ctTree.m_iBCShift[ucVal - 32] = iOne;
			}
		}
	}

	return;
}

//******************BS shfit end**********************************/

//******************GS shfit**********************************/

void AdAcbm::AdAcbmGSShift()
{
	int iOne = 0;
	int iTwo = 0;

	for (iOne = 0; iOne < m_ctTree.m_iPtnCount; iOne ++) 
	{
		for (iTwo = 0; iTwo < m_ctTree.m_iPtnCount; iTwo ++) 
		{
			AdAcbmSetGSShift2(m_ctTree.m_vStr[iOne], m_ctTree.m_vStr[iTwo]);
		}
	}

	return;
}

int AdAcbm::AdAcbmSetGSShift2(string & sStr1, string & sStr2)
{
	int iCount1 = 0;
	int iCount2 = 0;
	int iIndex1 = 0;
	int iIndex2 = 0;
	int iOffset = 0;
	UCHAR ucFirst = '\0';

	ucFirst = static_cast<UCHAR>(sStr1[0]);
	ucFirst = tolower(ucFirst);

	for (iCount1 = 1; iCount1 < static_cast<int>(sStr2.size()); iCount1 ++) 
	{
		if (ucFirst != tolower(static_cast<UCHAR>(sStr2[iCount1]))) 
		{
			break;
		}
	}

	AdAcbmSetGSShift1(sStr1, 1, iCount1);

	iCount1 = 1;
	while (true) 
	{
		while ((iCount1 < static_cast<int>(sStr2.size())) && (ucFirst != tolower(static_cast<UCHAR>(sStr2[iCount1])))) 
			iCount1 ++;

		if (iCount1 == static_cast<int>(sStr2.size())) 
			break;

		iIndex1 = 0;
		iIndex2 = iCount1;
		iOffset = iCount1;
		if (iOffset > m_ctTree.m_iMinPtnSize) 
			break;

		while ((iIndex1 < static_cast<int>(sStr1.size())) 
				&& (iIndex2 < static_cast<int>(sStr2.size()))) 
		{
			if (tolower(sStr1[iIndex1 ++]) != tolower(sStr2[iIndex2 ++])) 
			{
				break;
			}
		}

		if (iIndex2 == static_cast<int>(sStr2.size())) 
		{
			for (iCount2 = iIndex1; iCount2 < static_cast<int>(sStr1.size()); iCount2 ++) 
			{
				AdAcbmSetGSShift1(sStr1, iCount2 + 1, iOffset);
			}
		}
		else 
		{
			AdAcbmSetGSShift1(sStr1, iIndex1 + 1, iOffset);
		}

		iCount1 ++;
	}

	return AD_SUCCESS;
}

int AdAcbm::AdAcbmSetGSShift1(string & sStr, int iDepth, int iShift)
{
	int iCount = 0;
	ADACBMNODE_S* pstNode = m_ctTree.m_pstRoot;

	for (iCount = 0; iCount < iDepth; iCount ++) 
	{
		pstNode = pstNode->m_pstChild[static_cast<UCHAR>(sStr[iCount])];
		if (NULL == pstNode) 
		{
			goto AdError;
		}
	}

	pstNode->m_iGSShift = pstNode->m_iGSShift < iShift ? pstNode->m_iGSShift : iShift;

	return AD_SUCCESS;

AdError:
	return AD_FAILURE;
}

//******************GS shfit end**********************************/
//******************GS init**********************************/

void AdAcbm::AdAcbmGSShiftInit()
{
	AdAcbmGSShiftInitCore(m_ctTree.m_pstRoot, m_ctTree.m_iMinPtnSize);

	return;
}

int AdAcbm::AdAcbmGSShiftInitCore(ADACBMNODE_S* pstRoot, int iShift)
{
	int iCount = 0;

	if (-2 != pstRoot->m_iLabel) 
	{
		pstRoot->m_iGSShift = iShift;
	}

	for (iCount = 0; iCount < AD_ACBM; iCount ++) 
	{
		if ((iCount >= 'A') && (iCount <= 'Z')) continue;
		if (NULL != pstRoot->m_pstChild[iCount]) 
		{
			AdAcbmGSShiftInitCore(pstRoot->m_pstChild[iCount], iShift);
		}
	}

	return AD_SUCCESS;
}

//******************GS init End**********************************/

int AdAcbm::AdAcbmTreeAdd(const string & sStr, void * pData)
{
	int iVal = 0;
	int iPtnLen = 0;
	UCHAR ucValue = '\0';
	ADACBMNODE_S* pstNode = NULL;
	ADACBMNODE_S* pstRoot = NULL;
	ADACBMNODE_S* pstParent = NULL;

	if(m_ctTree.m_pstRoot == NULL)
	{
		pstRoot = (ADACBMNODE_S*)malloc(sizeof(ADACBMNODE_S));
		if (NULL != pstRoot) 
		{
			memset(pstRoot, 0, sizeof(ADACBMNODE_S));
			pstRoot->m_iDepth = 0;
			pstRoot->m_iLabel = -2;
			m_ctTree.m_pstRoot = pstRoot;
		}
	}
	else
	{
		pstRoot = m_ctTree.m_pstRoot;
	}

	if ((NULL != pstRoot)  && ((iPtnLen = sStr.size()) > 0))
	{
		if (iPtnLen > m_ctTree.m_iMaxDepth) 
		{
			m_ctTree.m_iMaxDepth = iPtnLen;
		}
		if (iPtnLen < m_ctTree.m_iMinPtnSize) 
		{
			m_ctTree.m_iMinPtnSize = iPtnLen;
		}

		pstParent = pstRoot;
		for (iVal = 0; iVal < iPtnLen; iVal ++) 
		{
			ucValue = static_cast<unsigned char>(sStr[iVal]);
			ucValue = tolower(ucValue);
			if (NULL == pstParent->m_pstChild[ucValue]) 
			{
				break;
			}
			pstParent = pstParent->m_pstChild[ucValue];
		}

		if (iVal < iPtnLen) 
		{
			for (; iVal < iPtnLen; iVal ++) 
			{
				ucValue = static_cast<unsigned char>(sStr[iVal]);
				ucValue = tolower(ucValue);
				pstNode = (ADACBMNODE_S*)malloc(sizeof(ADACBMNODE_S));
				if (NULL != pstNode) 
				{
					memset(pstNode, 0, sizeof(ADACBMNODE_S));
					pstNode->m_iLabel = -1;
					pstNode->m_ucVal = ucValue;
					pstNode->m_iDepth = iVal + 1;

					pstParent->m_pstChild[ucValue] = pstNode;
					if ((ucValue >= 'a') && (ucValue <= 'z')) 
					{
						pstParent->m_pstChild[ucValue - 32] = pstNode;
					}
					pstParent->m_ucChild = ucValue;
					pstParent->m_iChild ++;

					pstNode->m_pstParent = pstParent;
					pstParent = pstNode;
				}
				else 
				{
					goto AdError;
				}
			}
		}
		if(pstParent->m_iLabel < 0)
		{
			pstParent->m_iLabel = m_ctTree.m_iPtnCount;
			m_ctTree.m_iPtnCount ++;
		}

		m_ctTree.m_vStr.push_back(sStr);
		m_ctTree.m_mData[pstParent->m_iLabel].push_back( pData);
		
	}
	else 
	{
		goto AdError;
	}

	return AD_SUCCESS;

AdError:
	m_ctTree.clear();

	return AD_FAILURE;
}


int AdAcbm::AdAcbmSearch(const string & sStr, map<void *, list<string> > & mResult)
{
	int iMatch = 0;
	int iCur = 0;
	int iBase = 0;
	int iShift = 0;
	int iGSShift = 0;
	int iBCShift = 0;
	UCHAR ucValue;
	ADACBMNODE_S* pstNode = NULL;
	map<int, list<void *> >::iterator  mIter;
	list<void *>::iterator  lIter;
	
	if (static_cast<int>(sStr.size()) < m_ctTree.m_iMinPtnSize) 
		goto AdReturn;

	iBase = static_cast<int>(sStr.size()) - m_ctTree.m_iMinPtnSize;
	while (iBase >= 0) 
	{
		iCur = iBase;
		pstNode = m_ctTree.m_pstRoot;
		ucValue = static_cast<unsigned char>(sStr[iCur]);

		while (NULL != pstNode->m_pstChild[ucValue]) 
		{
			pstNode = pstNode->m_pstChild[ucValue];

			if (pstNode->m_iLabel >= 0) 
			{
				mIter = m_ctTree.m_mData.find(pstNode->m_iLabel);
				if(mIter != m_ctTree.m_mData.end())
				{
					for(lIter = (mIter->second).begin(); lIter != (mIter->second).end();  lIter ++)
					{
						mResult[*lIter].push_back(m_ctTree.m_vStr[pstNode->m_iLabel]);
					}
				}
				iMatch += 1;
			}

			if ((++ iCur) >= static_cast<int>(sStr.size())) 
				break;
			ucValue = static_cast<unsigned char>(sStr[iCur]);
		}
		
		if (pstNode->m_iChild > 0)
		{
			iGSShift = pstNode->m_pstChild[pstNode->m_ucChild]->m_iGSShift;
			if (iCur < static_cast<int>(sStr.size())) 
			{
				iBCShift = m_ctTree.m_iBCShift[ucValue] + iBase - iCur;
			}
			else 
			{
				iBCShift = 1;
			}
			iShift = iGSShift > iBCShift ? iBCShift : iGSShift;
			if (iShift <= 0 ) 
				iShift = 1;
			iBase -= iShift;
		}
		else {
			iBase --;
		}
	}

AdReturn:
	return iMatch;
}

void AdAcbm::AdAcbmDisplayMatch(int iNum, int base)
{
//	DebugMessage("Index:", iNum);
//	DebugMessage("Offset:", base);
//	DebugMessage("Keyword:", m_ctTree.m_vStr[iNum].c_str());
//	DebugMessage("");
	return;
}
