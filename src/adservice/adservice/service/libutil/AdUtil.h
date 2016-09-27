/***************************************
*	Description:  util module
*	Author: wilson
*	Date: 2015/11/13
*
*	CopyRight: Adirect Technology Co, Ltd
*
****************************************/
#ifndef  __AD_UTIL_H_
#define __AD_UTIL_H_

#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>

#define AD_CACHE_MAX    AD_KBYTES8

class  AdCache
{
public:
	AdCache(AdCache & ctChache)
	{
		m_iMaxLen = AD_CACHE_MAX - 1;
		*this = ctChache;
	};
	AdCache(const char * pBuf,  int iLen)
	{
		m_iMaxLen = AD_CACHE_MAX - 1;
		Write(pBuf, iLen);
	};
	AdCache()
	{
		m_iMaxLen = AD_CACHE_MAX - 1;
		m_iLen = 0;
	};
	~AdCache(){};
	void Clear()
	{
		m_iLen = 0;
	};
	char * Get()
	{
		return m_cCache;
	};
	int Max()
	{
		return AD_CACHE_MAX;
	};

	int Size()
	{
		return m_iLen;
	};
	
	void Set(int iLen)
	{
		m_iLen = iLen;
	};

	int Write(const char * pBuff, int iLen)
	{
		if(iLen > m_iMaxLen)
		{
			memcpy(m_cCache, pBuff, m_iMaxLen);
			m_iLen = m_iMaxLen;
			m_cCache[m_iMaxLen] = '\0';
		}
		else
		{
			memcpy(m_cCache, pBuff, iLen);
			m_iLen = iLen;
			m_cCache[iLen] = '\0';
		}
		return AD_SUCCESS;
	};

	int Append(char * buff, int iLen)
	{
		if(iLen + m_iLen > m_iMaxLen)
		{
			memcpy(m_cCache + m_iLen,  buff, m_iMaxLen - m_iLen);
			m_iLen = m_iMaxLen;
			m_cCache[m_iMaxLen] = '\0';
			return AD_FAILURE;
		}
		memcpy(m_cCache + m_iLen,  buff, iLen);
		m_iLen += iLen;
		m_cCache[m_iLen] = '\0';
		return AD_SUCCESS;
	};

	int Append(const char * buff, int iLen)
	{
		if(iLen + m_iLen > m_iMaxLen)
		{
			memcpy(m_cCache + m_iLen,  buff, m_iMaxLen - m_iLen);
			m_iLen = m_iMaxLen;
			m_cCache[m_iMaxLen] = '\0';
			return AD_FAILURE;
		}
		memcpy(m_cCache + m_iLen,  buff, iLen);
		m_iLen += iLen;
		m_cCache[m_iLen] = '\0';
		return AD_SUCCESS;
	};
	
	AdCache & operator=(const AdCache &a) 
	{
		if(a.m_iLen > m_iMaxLen)
		{
			memcpy(this->m_cCache,  a.m_cCache, m_iMaxLen);
			this->m_iLen = m_iMaxLen;
			this->m_cCache[this->m_iLen] = '\0';
		}
		else
		{
			memcpy(this->m_cCache,  a.m_cCache, a.m_iLen);
			this->m_iLen = a.m_iLen;
			this->m_cCache[this->m_iLen] = '\0';
		}
		return *this;
	};
private:
	char m_cCache[AD_CACHE_MAX];
	int m_iLen;
	int m_iMaxLen;
};


class AdString
{
public:
	static void StrSplit(const std::string & str, std::vector<std::string>& ret_, const std::string  sep)
	{
		std::string tmp;
		std::string::size_type pos_begin = str.find_first_not_of(sep);
		std::string::size_type comma_pos = 0;
	
		while (pos_begin != std::string::npos)
		{
			comma_pos = str.find(sep, pos_begin);
			if (comma_pos != std::string::npos)
			{
				tmp = str.substr(pos_begin, comma_pos - pos_begin);
				pos_begin = comma_pos + sep.length();
			}
			else
			{
				tmp = str.substr(pos_begin);
				pos_begin = comma_pos;
			}
	
			if (!tmp.empty())
			{
				ret_.push_back(tmp);
				tmp.clear();
			}
		}
	};

	static void StrReplace(const std::string & sSrc, const std::string & sMatch, 
		const std::string & sNew, std::string & sDes)
	{
		std::string::size_type pos_begin =0;
		std::string::size_type comma_pos = 0;
	
		while (pos_begin != std::string::npos)
		{
			comma_pos = sSrc.find(sMatch, pos_begin);
			if (comma_pos != std::string::npos)
			{
				sDes += sSrc.substr(pos_begin, comma_pos - pos_begin);
				sDes += sNew;
				pos_begin = comma_pos + sMatch.length();
			}
			else
			{
				sDes += sSrc.substr(pos_begin);
				pos_begin = comma_pos;
			}
		}
	};
	
};

enum Http_Method
{
	HTTP_GET = 1,
	HTTP_POST,
	HTTP_HEAD,
	HTTP_CONNECT,
	HTTP_PUT,
	HTTP_DELETE,
	HTTP_OPTIONS,
	HTTP_PROPFIND
};
	
enum Http_Response
{
	HTTP_OK = 200,
	HTTP_NO_CONTENT = 204,
	HTTP_MOVED_PERMANENTLY = 301,
	HTTP_MOV_TEMP = 302,
	HTTP_BAD_REQ  = 400,
	HTTP_UNAUTH = 401,
	HTTP_NO_FOUND = 404,
	HTTP_REQ_TIMEOUT = 408,
	HTTP_TOO_LARGE =413
};

class AdHttp
{
public:

	AdHttp()
	{
		m_mMethodToInt["GET"] = HTTP_GET;
		m_mMethodToInt["POST"] = HTTP_POST;
		m_mMethodToInt["HEAD"] = HTTP_HEAD;
		m_mMethodToInt["CONNECT"] = HTTP_CONNECT;
		m_mMethodToInt["PUT"] = HTTP_PUT;
		m_mMethodToInt["DELETE"] = HTTP_DELETE;
		m_mMethodToInt["OPTIONS"] = HTTP_OPTIONS;
		m_mMethodToInt["PROPFIND"] = HTTP_PROPFIND;

		m_mMethodToStr[HTTP_GET] = "GET";
		m_mMethodToStr[HTTP_POST] = "POST";
		m_mMethodToStr[HTTP_HEAD] = "HEAD";
		m_mMethodToStr[HTTP_CONNECT] = "CONNECT";
		m_mMethodToStr[HTTP_PUT] = "PUT";
		m_mMethodToStr[HTTP_DELETE] = "DELETE";
		m_mMethodToStr[HTTP_OPTIONS] = "OPTIONS";
		m_mMethodToStr[HTTP_PROPFIND] = "PROPFIND";

		
		m_mResponse[HTTP_OK] = "Ok";
		m_mResponse[HTTP_NO_CONTENT] = "No Content";
		m_mResponse[HTTP_MOVED_PERMANENTLY] = "Moved Permanently";
		m_mResponse[HTTP_MOV_TEMP] = "Move temporarily";
		m_mResponse[HTTP_BAD_REQ] = "Bad Request";
		m_mResponse[HTTP_UNAUTH] = "Unauthorized";
		m_mResponse[HTTP_NO_FOUND] = "Not Found";
		m_mResponse[HTTP_REQ_TIMEOUT] = "Request Timeout";
		m_mResponse[HTTP_TOO_LARGE] = " Request Entity Too Large";
	};

	virtual ~AdHttp()
	{
		Clear();
	};
	
	static int GetAttr(AdCache & ctCache, const char * pAttr, std::string & sValue)
	{
		char *pTmp;
		char ch;
		char * pData =ctCache.Get();
		int iLen = ctCache.Size();
		int attrLen = strlen(pAttr);
		int i;

		//find Attr
		pTmp = strcasestr(pData, pAttr);
		if (NULL == pTmp) 
		{
			return AD_FAILURE;
		}
		pTmp += attrLen;
		
		iLen -= pTmp - pData;
		i = 0;
		while(i <= iLen)
		{
		        ch = *(pTmp + i);
		        if((ch == '?') || (ch == ' ') || (ch == '\n') || (ch == '\t') || (ch =='\0') ||(ch == '\r'))
			{
				break;
			}
			i ++;
		}
		if(i > iLen)
			return AD_FAILURE;

		sValue.assign(pTmp, i);

		return AD_SUCCESS;
	};

	static int GetAttr(AdCache & ctCache, std::string & ctAttr, std::string & sValue)
	{
		char *pTmp;
		char ch;
		char * pData =ctCache.Get();
		int iLen = ctCache.Size();
		int attrLen = strlen(ctAttr.c_str());
		int i;

		//find Attr
		pTmp = strcasestr(pData, ctAttr.c_str());
		if (NULL == pTmp) 
		{
			return AD_FAILURE;
		}
		pTmp += attrLen;
		
		iLen -= pTmp - pData;
		i = 0;
		while(i <= iLen)
		{
		        ch = *(pTmp + i);
		        if((ch == '?') || (ch == ' ') || (ch == '\n') || (ch == '\t') || (ch =='\0') ||(ch == '\r'))
			{
				break;
			}
			i ++;
		}
		if(i > iLen)
			return AD_FAILURE;

		sValue.assign(pTmp, i);

		return AD_SUCCESS;
	};

	static inline int GetPostUri(AdCache & ctCache, std::string & sValue)
	{
		return GetAttr(ctCache,  "POST ", sValue);
	};

	static inline int GetGetUri(AdCache & ctCache, std::string & sValue)
	{
		return GetAttr(ctCache,  "GET ", sValue);
	};


	static inline int GetHost(AdCache & ctCache, std::string & sValue)
	{
		return GetAttr(ctCache,  "Host: ", sValue);
	};

	static inline int GetLen(AdCache & ctCache)
	{
		std::string  sValue;
		int ret = GetAttr(ctCache, "Content-Length: ", sValue);
		if(ret != AD_SUCCESS)
			return ret;
		if(sValue.empty())
			return AD_FAILURE;
		
		return atoi(sValue.c_str());
	};
	
	static inline char * GetBody(AdCache & ctCache)
	{
		char * pBody;
		pBody = strcasestr(ctCache.Get(), "\r\n\r\n");
		if (NULL == pBody) 
		{
			return pBody;
		}
		
		pBody += strlen("\r\n\r\n");
		return pBody;
	};

	char * GetBody(bool bRes = false)
	{
		if(bRes)
			return m_pResBody;
		else
			return m_pBody;
	};

	int GetUri(int iMethod, std::string & sValue)
	{
		if((iMethod < 0) || (m_iMethod == iMethod))
		{
			sValue = m_sUri;
			return AD_SUCCESS;
		}
		return AD_FAILURE;
	};

	int GetHeader(const char *  pHeader, std::string & sValue)
	{
		std::map<std::string, std::string>::iterator iter;
		
		iter = m_mAttrs.find(pHeader);
		if(iter == m_mAttrs.end())
			return AD_FAILURE;

		sValue = iter->second;
		return AD_SUCCESS;
	};

	int GetPara(const char *  pPara, std::string & sValue)
	{
		std::map<std::string, std::string>::iterator iter;
		
		iter = m_mParas.find(pPara);
		if(iter == m_mParas.end())
			return AD_FAILURE;

		sValue = iter->second;
		return AD_SUCCESS;
	};

	inline int GetHost(std::string & sValue)
	{
		return GetHeader("Host", sValue);
	};

	int GetLen(bool bRes = false)
	{
		std::map<std::string, std::string>::iterator iter;

		if(bRes)
		{
			iter = m_mAttrs.find("Content-Length");
			if(iter == m_mAttrs.end())
				return AD_FAILURE;
		}
		else
		{
			iter = m_mResAttrs.find("Content-Length");
			if(iter == m_mResAttrs.end())
				return AD_FAILURE;
		}

		return atoi(iter->second.c_str());
	};

	std::string & GetStatus()
	{
		return m_sStatus;
	};

	
	static char *SkipQuoted(char **buf, const char *delimiters, const char *whitespace, char quotechar)
	{
		char *p, *begin_word, *end_word, *end_whitespace;
	
		begin_word = *buf;
		end_word = begin_word + strcspn(begin_word, delimiters);
	
		// Check for quotechar
		if (end_word > begin_word) {
			p = end_word - 1;
			while (*p == quotechar) {
				// If there is anything beyond end_word, copy it
				if (*end_word == '\0') {
					*p = '\0';
					break;
				} else {
					size_t end_off = strcspn(end_word + 1, delimiters);
					memmove (p, end_word, end_off + 1);
					p += end_off; // p must correspond to end_word - 1
					end_word += end_off + 1;
				}
			}
			for (p++; p < end_word; p++) {
				*p = '\0';
			}
		}
	
		if (*end_word == '\0') {
			*buf = end_word;
		} else {
			end_whitespace = end_word + 1 + strspn(end_word + 1, whitespace);
	
			for (p = end_word; p < end_whitespace; p++) {
				*p = '\0';
			}
	
			*buf = end_whitespace;
		}
	
		return begin_word;
	};
	
	int Parse(AdCache & ctCache) 
	{
		CHAR8  * cBuff = ctCache.Get();
		CHAR8 * pBuff;
		CHAR8  cStatus[AD_KBYTES4];
		INT32   iLen = ctCache.Size();
		std::vector<std::string> vStr;

		m_pBody= GetBody(ctCache);
		if(m_pBody != NULL)
		{
			m_iHeadLen = m_pBody - cBuff;
		}

		cBuff[m_iHeadLen - 1] = '\0';
	
		// RFC says that all initial whitespaces should be ingored
		while (*cBuff != '\0' && isspace(* (UCHAR *) cBuff)) {
			cBuff++;
		}

		//get method
		m_sMethod = SkipQuoted(&cBuff, " ", " ", 0);
		m_sUri = SkipQuoted(&cBuff, " ", " ", 0);
		//check is valid uri
		if((m_sUri[0] != '/') && (m_sUri[0] != '*') && (m_sUri[0] == '*' && m_sUri[1] != '\0'))
		{
			snprintf(cStatus, AD_KBYTES4, "Cannot parse HTTP request: [%s]\n", ctCache.Get());
			Error(HTTP_BAD_REQ, cStatus);
			return AD_FAILURE;
		}
		m_sHttpVersion = SkipQuoted(&cBuff, "\r\n", "\r\n", 0);

#if 0
		if(m_sHttpVersion != "HTTP/1.1")
		{
			return AD_FAILURE;
		}
#endif

		//parse method
		std::map<std::string, INT32>::iterator iter;
		iter = m_mMethodToInt.find(m_sMethod);
		if(m_mMethodToInt.end() == iter)
		{
			snprintf(cStatus, AD_KBYTES4, "Cannot parse HTTP request: [%s]", ctCache.Get());
			Error(HTTP_BAD_REQ,cStatus);
			return AD_FAILURE;
		}
		m_iMethod = iter->second;

		AdString::StrSplit(m_sUri, vStr, "?");
		if(vStr.size() == 2)
		{
			m_sUri = vStr[0];
			ParsePara(vStr[1]);
		}

		while(ctCache.Get() -cBuff < iLen)
		{
			pBuff = SkipQuoted(&cBuff, ":", " ", 0);
			if(*pBuff == '\0')
			{
				break;
			}
			m_mAttrs[pBuff] = SkipQuoted(&cBuff, "\r\n", "\r\n", 0);
		}

		m_iBodyLen = GetLen();

		if(m_iHeadLen + m_iBodyLen > iLen)
		{
			Error(HTTP_TOO_LARGE, "");
			return AD_FAILURE;
		}
	
		return m_iMethod;
	};

	int ParseResponse(AdCache & ctCache) 
	{
		CHAR8  * cBuff = ctCache.Get();
		CHAR8 * pBuff;
		INT32   iLen = ctCache.Size();
		std::vector<std::string> vStr;

		m_pResBody= GetBody(ctCache);
		if(m_pResBody != NULL)
		{
			m_iResHeadLen = m_pResBody - cBuff;
		}
		else
			m_iResHeadLen = iLen;

		cBuff[m_iResHeadLen - 1] = '\0';
	
		// RFC says that all initial whitespaces should be ingored
		while (*cBuff != '\0' && isspace(* (UCHAR *) cBuff)) {
			cBuff++;
		}

		//get Version
		std::string sVersion = SkipQuoted(&cBuff, " ", " ", 0);
		m_iResState = atoi(SkipQuoted(&cBuff, " ", " ", 0));
		std::string sState = SkipQuoted(&cBuff, "\r\n", "\r\n", 0);

	//	if(sVersion != "HTTP/1.1")
	//		return AD_FAILURE;
		
		//check state
		std::map<INT32, std::string>::iterator iter;
		iter = m_mResponse.find(m_iResState);
		if(m_mResponse.end() == iter)
		{
			return AD_FAILURE;
		}

		while(ctCache.Get() -cBuff < iLen)
		{
			pBuff = SkipQuoted(&cBuff, ":", " ", 0);
			if(*pBuff == '\0')
			{
				break;
			}
			m_mResAttrs[pBuff] = SkipQuoted(&cBuff, "\r\n", "\r\n", 0);
		}

		m_iResBodyLen = GetLen();

		if(m_iResHeadLen + m_iResBodyLen > iLen)
		{
			return AD_FAILURE;
		}
	
		return m_iResState;
	};


	void Clear()
	{
		//request clear
		m_sMethod.clear();
		m_iMethod = AD_FAILURE;
		m_iHeadLen = AD_FAILURE;
		m_iBodyLen = AD_FAILURE;
		m_pBody = NULL;
		m_sUri.clear();
		m_mParas.clear();
		m_sHttpVersion.clear();
		m_mAttrs.clear();

		//reqeust parse status
		m_sStatus.clear();

		//response clear
		m_iResState = AD_FAILURE;
		m_iResHeadLen = AD_FAILURE;
		m_iResBodyLen = AD_FAILURE;
		m_pResBody = NULL;
		m_mResAttrs.clear();
	};

	void Error(int iStatus, const char * cExt) 
	{
		char cBuf[AD_KBYTES4];
		char cBody[AD_KBYTES2];
		int iLen;


		cBody[0] = '\0';

		// Errors 1xx, 204 and 304 MUST NOT send a body
		if (iStatus > 199 && iStatus != 204 && iStatus != 304) 
		{
			iLen = snprintf(cBody, AD_KBYTES2, "Error %d: %s", iStatus, m_mResponse[iStatus].c_str());
			cBody[iLen++] = '\n';

			iLen += snprintf(cBody + iLen, AD_KBYTES2 - iLen,"%s", cExt);

			snprintf(cBuf, AD_KBYTES4, "HTTP/1.1 %d %s\r\n"
				"Content-Type: text/plain\r\n"
				"Content-Length: %d\r\n"
				"Connection: close\r\n\r\n%s", iStatus, m_mResponse[iStatus].c_str(), iLen, cBody);
		}
		else
		{
			snprintf(cBuf, AD_KBYTES4, "HTTP/1.1 %d %s\r\n"
					"Content-Type: text/plain\r\n"
					"Content-Length: 0\r\n"
					"Connection: close\r\n\r\n", iStatus, m_mResponse[iStatus].c_str());
		}
		
		m_sStatus = cBuf;
	};

	void GetResponse(int iStatus, const char * pType, AdCache & cCache) 
	{
		int iLen;

		if(pType != NULL)
		{
			iLen = snprintf(cCache.Get(), cCache.Max(), "HTTP/1.1 %d %s\r\n"
				"Content-Type: %s\r\n"
				"Content-Length: 0\r\n"
				"Connection: close\r\n\r\n", iStatus, m_mResponse[iStatus].c_str(),  
				pType);
		}
		else
		{
			iLen = snprintf(cCache.Get(), cCache.Max(), "HTTP/1.1 %d %s\r\n"
				"Content-Type: text/plain\r\n"
				"Content-Length: 0\r\n"
				"Connection: close\r\n\r\n", iStatus, m_mResponse[iStatus].c_str());
		}

		cCache.Set(iLen);
	};

	void GetRedirect(int iStatus, std::string & sLocation, AdCache & cCache) 
	{
		int iLen;
	
		if(!sLocation.empty())
		{
			iLen = snprintf(cCache.Get(), cCache.Max(), "HTTP/1.1 %d %s\r\n"
				"Content-Type: text/plain\r\n"
				"Content-Length: 0\r\n"
				"Location: %s\r\n"
				"Connection: close\r\n\r\n", iStatus, m_mResponse[iStatus].c_str(), sLocation.c_str());
			cCache.Set(iLen);
		}
	
	};

	
	void GetRequest(int iMethod, const char * pUri,const char * pBody, INT32 iBodyLen, AdCache & cCache) 
	{
		int iLen;
	
		if(pUri != NULL)
		{
			iLen = snprintf(cCache.Get(), cCache.Max(), "%s %s HTTP/1.1\r\n"
				"Content-Type: application/Json\r\n"
				"Content-Length: %d\r\n"
				"Connection: close\r\n\r\n%s", m_mMethodToStr[iMethod].c_str(), pUri, iBodyLen, pBody);
		}
		else
		{
			iLen = snprintf(cCache.Get(), cCache.Max(), "%s  / HTTP/1.1\r\n"
				"Content-Type:application/Json\r\n"
				"Content-Length: %d\r\n"
				"Connection: close\r\n\r\n%s", m_mMethodToStr[iMethod].c_str(), iBodyLen, pBody);
		}
	
		cCache.Set(iLen);
	};

	void GetResponse(int iStatus, const char * pType, const char * pBody, INT32 iBodyLen, AdCache & cCache) 
	{
		int iLen;

		if(pType != NULL)
		{
			iLen = snprintf(cCache.Get(), cCache.Max(), "HTTP/1.1 %d %s\r\n"
				"Content-Type: %s\r\n"
				"Content-Length: %d\r\n"
				"Connection: close\r\n\r\n%s", iStatus, m_mResponse[iStatus].c_str(),  
				pType, iBodyLen, pBody);
		}
		else
		{
			iLen = snprintf(cCache.Get(), cCache.Max(), "HTTP/1.1 %d %s\r\n"
				"Content-Type: text/plain\r\n"
				"Content-Length: %d\r\n"
				"Connection: close\r\n\r\n%s", iStatus, m_mResponse[iStatus].c_str(),  
				 iBodyLen, pBody);
		}

		cCache.Set(iLen);
	};


	bool CheckResState(INT32 res)
	{
		return m_iResState == res;
	}


private:
	//request Para
	std::string m_sMethod;
	INT32  m_iMethod;
	INT32  m_iHeadLen;
	INT32  m_iBodyLen;
	CHAR8* m_pBody;
	std::string m_sUri;
	std::map<std::string, std::string> m_mParas;
	std::string m_sHttpVersion;
	std::map<std::string, std::string> m_mAttrs;
	std::string m_sStatus;

	//response
	INT32 m_iResState;
	INT32  m_iResHeadLen;
	INT32  m_iResBodyLen;
	CHAR8* m_pResBody;
	std::map<std::string, std::string> m_mResAttrs;
	

	std::map<std::string, INT32> m_mMethodToInt;
	std::map<INT32, std::string> m_mMethodToStr;
	std::map<INT32, std::string> m_mResponse;

	int ParsePara(std::string & sStr)
	{
		std::string::size_type iNameBegin = sStr.find_first_not_of("=");
		std::string::size_type iNameEnd = 0;
		std::string::size_type iValueBegin = 0;
		std::string::size_type iValueEnd = 0;
		
		while (iNameBegin != std::string::npos)
		{
			iNameEnd = sStr.find("=", iNameBegin);
			if (iNameEnd != std::string::npos)
			{
				iValueBegin =iNameEnd + 1;
				iValueEnd = sStr.find("&", iValueBegin);
				if (iValueEnd != std::string::npos)
				{
					m_mParas[sStr.substr(iNameBegin, iNameEnd - iNameBegin)] = 
						sStr.substr(iValueBegin, iValueEnd - iValueBegin);
					iNameBegin = iValueEnd + 1;
				}
				else
				{
					m_mParas[sStr.substr(iNameBegin, iNameEnd - iNameBegin)] = 
						sStr.substr(iValueBegin);
					break;
				}
			}
			else
			{
				break;
			}
		}
		return AD_SUCCESS;
	}
};
#endif

