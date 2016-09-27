/***************************************
*	Description: class Admd5
*	Author: gong_libin
*	Date: 2013_09_11
*
*	CopyRight: Adirect Technology Co, Ltd
*
****************************************/
#ifndef _AD_MD5_H
#define	_AD_MD5_H

#include "AdGlobal.h"

#define		AD_MD5			AD_BYTE16

#define F(x,y,z) ((x & y) | (~x & z))
#define G(x,y,z) ((x & z) | (y & ~z))
#define H(x,y,z) (x^y^z)
#define I(x,y,z) (y ^ (x | ~z))
#define ROTATE_LEFT(x,n) ((x << n) | (x >> (32-n)))
#define FF(a,b,c,d,x,s,ac) \
          { \
          a += F(b,c,d) + x + ac; \
          a = ROTATE_LEFT(a,s); \
          a += b; \
          }
#define GG(a,b,c,d,x,s,ac) \
          { \
          a += G(b,c,d) + x + ac; \
          a = ROTATE_LEFT(a,s); \
          a += b; \
          }
#define HH(a,b,c,d,x,s,ac) \
          { \
          a += H(b,c,d) + x + ac; \
          a = ROTATE_LEFT(a,s); \
          a += b; \
          }
#define II(a,b,c,d,x,s,ac) \
          { \
          a += I(b,c,d) + x + ac; \
          a = ROTATE_LEFT(a,s); \
          a += b; \
          }                                            

class AdMd5
{
public:
	AdMd5();
	virtual ~AdMd5();
	void AdMd5Run(char* src, unsigned char* dst);
	void AdMd5Run(char *src, int len, unsigned char *dst);

private:

	unsigned int count[2];
    	unsigned int state[4];
    	unsigned char buffer[AD_BYTE64];   
		
	void Md5Transform(unsigned int state[4],unsigned char block[64]);
	void Md5Init();
	void Md5Final(unsigned char digest[16]);
	void Md5Update(unsigned char *input,unsigned int inputlen);
	void Md5Encode(unsigned char *output,unsigned int *input,unsigned int len);
	void Md5Decode(unsigned int *output,unsigned char *input,unsigned int len);
};

#endif 
