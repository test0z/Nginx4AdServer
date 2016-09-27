/***************************************
*	Description: Ad Charge System, check bid Uinit money.
*	Author: wilson
*	Date: 2016/01/28
*
*	CopyRight: Adirect Technology Co, Ltd
*
****************************************/

#ifndef _AD_TIME_H_
#define _AD_TIME_H_
#include <iostream>
#include <map>
#include <list>

#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#include "AdGlobal.h"

using namespace std;

typedef  time_t  Time_t;

class  Time
{
public:
	Time()
	{
		update();
	};
	virtual ~Time(){};
	void update()
	{
		gettimeofday(&m_stTime, NULL);	
	};

	ULONG getTime()
	{
		return m_stTime.tv_sec * 1000000 + m_stTime.tv_usec;
	}
	
	Time_t  getSecond()
	{
		return m_stTime.tv_sec;
	};

	int  getHour()
	{
		char pszDayTime[AD_BYTE32] = {0};
	   	struct tm  stTime;

		localtime_r(&m_stTime.tv_sec, &stTime);
		strftime(pszDayTime, 16, "%H", &stTime);
		return atoi(pszDayTime);
	};

	int  getHour(string & sHour)
	{
		char pszDayTime[AD_BYTE32] = {0};
	   	struct tm  stTime;

		localtime_r(&m_stTime.tv_sec, &stTime);
		strftime(pszDayTime, 16, "%H", &stTime);
		sHour = pszDayTime;
		return atoi(pszDayTime);
	};

	int getWeekDay()
	{
		char pszDayTime[AD_BYTE32] = {0};
	   	struct tm  stTime;

		localtime_r(&m_stTime.tv_sec, &stTime);
		strftime(pszDayTime, 16, "%w", &stTime);
		return atoi(pszDayTime);
	};

	int getDay(string & sDay)
	{
		char pszDayTime[AD_BYTE32] = {0};
	   	struct tm  stTime;

		localtime_r(&m_stTime.tv_sec, &stTime);
		strftime(pszDayTime, 16, "%F", &stTime);
		sDay = pszDayTime;
		return AD_SUCCESS;
	};

	int getYear()
	{
		char pszDayTime[AD_BYTE32] = {0};
	   	struct tm  stTime;

		localtime_r(&m_stTime.tv_sec, &stTime);
		strftime(pszDayTime, 16, "%G", &stTime);
		return atoi(pszDayTime);
	};

	int getYear(string & sYear)
	{
		char pszDayTime[AD_BYTE32] = {0};
	   	struct tm  stTime;

		localtime_r(&m_stTime.tv_sec, &stTime);
		strftime(pszDayTime, 16, "%G", &stTime);
		sYear = pszDayTime;
		return atoi(pszDayTime);
	};

	UINT toNow()
	{
		struct timeval stTime;
		gettimeofday(&stTime, NULL);
		if(stTime.tv_usec >= m_stTime.tv_usec)
			return (stTime.tv_sec - m_stTime.tv_sec) * 1000000 + stTime.tv_usec - m_stTime.tv_usec;
		else
			return (stTime.tv_sec - m_stTime.tv_sec) * 1000000  + m_stTime.tv_usec  - stTime.tv_usec;
	};

	static void wait(int iSecond)
	{
		if(iSecond > 0)
			sleep(iSecond);
	};

	static void waitNanoSec(int iNanoSec)
	{
		if(iNanoSec > 0)
			usleep(iNanoSec);
	};
	
private:
	struct timeval m_stTime;
};
#endif

