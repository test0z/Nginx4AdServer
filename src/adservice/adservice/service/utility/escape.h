//
// Created by guoze.lin on 16/2/24.
//

#ifndef ADCORE_ESCAPE_H
#define ADCORE_ESCAPE_H

#include <stddef.h>
#include <iostream>
#include <strings.h>
#include <cstring>
#include <sstream>
#include "common/types.h"

namespace adservice{
    namespace utility{
        namespace escape{

            //number should not be zero
            //这个数值encode算法灵感源自protobuf
            void numberEncode(int number,uint8_t* &buf);

            int numberDecode(const uint8_t* &buf);

            //aliyun的c++ api不支持二进制中有0,需要转义
            std::string encode4ali(const std::string& input);

            std::string encode4ali(const char* buf,int len);

            std::string decode4ali(const std::string& input);

            std::string decode4ali(const char* inputBuf,int len);

        }
    }
}

#endif //ADCORE_ESCAPE_H
