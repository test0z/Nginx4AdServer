#include <map>
#include <list>
#include <string>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "AdAcbm.h"
#include "AdGlobal.h"
#include "common/constants.h"



typedef map<void*, list<string> > UA_MAP_RESULT;

class AdUserAgentParser
{
public:
    AdUserAgentParser()
    {
        map<string, int> m_Global; 
        map<string, int>::iterator iter;
        m_Global["MSIE"] = SOLUTION_BROWSER_IE;
        m_Global["Edge"] = SOLUTION_BROWSER_EDGE;
        m_Global["360SE"] = SOLUTION_BROWSER_360;
        m_Global["Chrome"] = SOLUTION_BROWSER_CHROME;
        m_Global["Firefox"] = SOLUTION_BROWSER_FIREFOX;
        m_Global["Safari"] = SOLUTION_BROWSER_SAFARI;
        m_Global["Maxthon"] = SOLUTION_BROWSER_AOYOU;
        m_Global["QQBrowser"] = SOLUTION_BROWSER_QQ;
        m_Global["LBBROWSER"] = SOLUTION_BROWSER_LIEBAO;
        for(iter = m_Global.begin(); iter != m_Global.end(); iter++)
        {
            ac.AdAcbmTreeAdd( (string&)iter->first, (void*)iter->second);
        }
    }
    ~AdUserAgentParser(){};
    
    int parse(const string &ua, UA_MAP_RESULT &mResult)
    {
	    int ret = ac.AdAcbmSearch(ua, mResult);
		if(  0 == ret )
		{
			list<string> tmp;	
			mResult[(void*)SOLUTION_BROWSER_OTHER] = tmp;
		}
		return ret;
    };
    
private:
    AdAcbm ac;
    vector<string> vStr;
};

