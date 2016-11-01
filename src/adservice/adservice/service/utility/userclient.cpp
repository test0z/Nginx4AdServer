//
// Created by guoze.lin on 16/6/22.
//

#include "userclient.h"
#include "libutil/AdUaParser.h"
#include <assert.h>
#include <mtty/constants.h>

namespace adservice {
namespace utility {
    namespace userclient {

        class NumberStringTable {
        public:
            NumberStringTable(int b, int e)
            {
                begin = b;
                end = e;
                for (int i = b; i <= e; i++) {
                    arrays.push_back(std::to_string(i));
                }
            }
            const std::string & getString(int i)
            {
                assert(i >= begin && i <= end);
                return arrays[i - begin];
            }
            const char * get(int i)
            {
                assert(i >= begin && i <= end);
                return arrays[i - begin].data();
            }

        private:
            int begin;
            int end;
            std::vector<std::string> arrays;
        };

        int getOSTypeFromUA(const std::string & userAgent)
        {
            if (userAgent.find("NT ") != std::string::npos) {
                return SOLUTION_OS_WINDOWS;
            } else if (userAgent.find("Mac") != std::string::npos) {
                return SOLUTION_OS_MAC;
            } else if (userAgent.find("Linux") != std::string::npos) {
                return SOLUTION_OS_LINUX;
            }
            return SOLUTION_OS_OTHER;
        }

        std::string getBrowserTypeFromUA(const std::string & userAgent)
        {
            static AdUserAgentParser parser;
            static NumberStringTable t(SOLUTION_BROWSER_ALL, SOLUTION_BROWSER_OTHER);
            UA_MAP_RESULT result;
            parser.parse(userAgent, result);
            char buffer[256];
            char * p = buffer;
            for (auto iter = result.begin(); iter != result.end(); iter++) {
                *p++ = ',';
                const char * num = t.get((long)iter->first);
                while (*num != '\0') {
                    *p++ = *num++;
                }
            }
            *p = '\0';
            if (p == buffer) {
                return "";
            } else
                return std::string(buffer + 1);
        }
    }
}
}
