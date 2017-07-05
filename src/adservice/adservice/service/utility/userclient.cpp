//
// Created by guoze.lin on 16/6/22.
//

#include "userclient.h"
#include "libutil/AdUaParser.h"
#include <assert.h>
#include <boost/algorithm/string.hpp>
#include <mtty/constants.h>
#include <unordered_map>

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
                if (userAgent.find("iPhone") != std::string::npos || userAgent.find("iPad") != std::string::npos) {
                    return SOLUTION_OS_IOS;
                } else
                    return SOLUTION_OS_MAC;
            } else if (userAgent.find("Linux") != std::string::npos) {
                if (userAgent.find("Android") != std::string::npos) {
                    return SOLUTION_OS_ANDROID;
                } else
                    return SOLUTION_OS_LINUX;
            }
            return SOLUTION_OS_OTHER;
        }

        int getMobileTypeFromUA(const std::string & userAgent)
        {
            if (userAgent.find("iPad") != std::string::npos) { // pad
                return SOLUTION_DEVICE_IPAD;
            } else if (userAgent.find("iPhone ") != std::string::npos) { // iphone
                return SOLUTION_DEVICE_IPHONE;
            } else if (userAgent.find("Android") != std::string::npos) { // anroid
                if (userAgent.find("Mobile") != std::string::npos) {
                    return SOLUTION_DEVICE_ANDROID;
                }
                return SOLUTION_DEVICE_ANDROIDPAD;
            }
            return SOLUTION_DEVICE_OTHER;
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

    int getDeviceBrandFromUA(const std::string & userAgent)
    {
        std::vector<std::string> tmp;
        static std::unordered_map<std::string, int> brandFlag = { { "ipad", SOLUTION_BRAND_IPHONE },
                                                                  { "ios", SOLUTION_BRAND_IPHONE },
                                                                  { "iphone", SOLUTION_BRAND_IPHONE },
                                                                  { "huawei", SOLUTION_BRAND_HUAWEI },
                                                                  { "hw", SOLUTION_BRAND_HUAWEI },
                                                                  { "xiaomi", SOLUTION_BRAND_XIAOMI },
                                                                  { "mi", SOLUTION_BRAND_XIAOMI },
                                                                  { "miui", SOLUTION_BRAND_XIAOMI },
                                                                  { "oppo", SOLUTION_BRAND_OPPO },
                                                                  { "vivo", SOLUTION_BRAND_VIVO },
                                                                  { "meizu", SOLUTION_BRAND_MEIZU },
                                                                  { "mz", SOLUTION_BRAND_MEIZU },
                                                                  { "le", SOLUTION_BRAND_LESHI },
                                                                  { "letv", SOLUTION_BRAND_LESHI },
                                                                  { "samesung", SOLUTION_BRAND_SAMSUNG },
                                                                  { "sm", SOLUTION_BRAND_SAMSUNG },
                                                                  { "coolpad", SOLUTION_BRAND_COOLPAD },
                                                                  { "zte", SOLUTION_BRAND_COOLPAD },
                                                                  { "gn", SOLUTION_BRAND_GN },
                                                                  { "haier", SOLUTION_BRAND_HAIER },
                                                                  { "lenovo", SOLUTION_BRAND_LENOVO },
                                                                  { "yq", SOLUTION_BRAND_CHUIZI } };
        boost::split(tmp, userAgent, boost::is_any_of(" /;-()"));
        for (auto & s : tmp) {
            if (s.length() > 0) {
                auto iter = brandFlag.find(boost::to_lower<std::string>(input));
                if (iter != brandFlag.end()) {
                    return iter->second;
                }
            }
        }
        return SOLUTION_BRAND_ALL;
    }
}
}
