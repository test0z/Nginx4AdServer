/***************************************
*	Description: global head file, define base parameter.
*	Author: wilson
*	Date: 2015/07/17
*
*	CopyRight: Adirect Technology Co, Ltd
*
****************************************/

#ifndef _AD_GLOBAL__
#define _AD_GLOBAL__

#include <iostream>
#include <string>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* DAY_RANGE */
#define	AD_WEEK_DAYS			0x07
#define	AD_DAY_HOURS			24
#define	AD_DAY_SECONDS		(24*60*60)


#define AD_SUCCESS    0
#define AD_FAILURE    (-1)

#define  FLOAT   float
#define  DOUBLE  double
#define INT32   int
#define INT64   long
#define CHAR8    char
#define SHORT16   short

#define UINT     unsigned int
#define ULONG    unsigned long
#define UCHAR    unsigned char
#define USHORT   unsigned short


#define AD_BYTE16          	16
#define AD_BYTE32          	32
#define AD_BYTE64          	64
#define AD_BYTE128          	128
#define AD_BYTE256          	256
#define AD_BYTE512          	512


#define AD_KBYTES1             (1*1024)
#define AD_KBYTES2             (2*1024)
#define AD_KBYTES4             (4*1024)
#define AD_KBYTES8             (8*1024)


#define	AD_ERROR(format, args...)	do {				\
	fprintf(stderr, "[ERROR] %s %04d: ", __FILE__, __LINE__);	\
	fprintf(stderr, format, ##args);				\
} while (0)

#define	AD_INFO(format, args...)	do {				\
	time_t ulTime;											\
	struct tm ctTime;										\
	char buff[AD_BYTE128];									\
	::time(&ulTime);									\
	localtime_r(&ulTime, &ctTime);								\
	strftime(buff, AD_BYTE128, "%Y-%m-%d %H:%M:%S", &ctTime);         	\
	fprintf(stdout, "Time:    %s\n", buff);	\
	fprintf(stdout, format, ##args);				\
} while (0)

extern int AdRtbOutput(char * buff, int len);

#define	AD_PRINT(format, args...)	do {				\
	char buff[AD_KBYTES2];								\
	snprintf(buff, AD_KBYTES2, format, ##args);			\
	if(AdRtbOutput(buff, strlen(buff)) != AD_SUCCESS)				\
	{													\
		cout << buff;							\
	}  										\
} while (0)


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif




