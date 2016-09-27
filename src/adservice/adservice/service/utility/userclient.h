//
// Created by guoze.lin on 16/6/22.
//

#ifndef ADCORE_USERCLIENT_H
#define ADCORE_USERCLIENT_H

#include <string>

namespace adservice{
    namespace utility{
        namespace userclient{

            int getOSTypeFromUA(const std::string& userAgent);

            std::string getBrowserTypeFromUA(const std::string& userAgent);

        }
    }
}

#endif //ADCORE_USERCLIENT_H
