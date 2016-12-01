//
// Created by guoze.lin on 16/2/24.
//

#ifndef ADCORE_URL_H
#define ADCORE_URL_H

#include "common/types.h"
#include "muduo/net/http/HttpRequest.h"
#include <cstring>
#include <ctime>
#include <exception>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <stddef.h>
#include <strings.h>
#include <tuple>
#include <vector>

namespace adservice {
namespace utility {

    namespace url {

        std::string urlEncode(const std::string & input);

        std::string urlDecode(const std::string & input);

        /**
         * 快速url encode
         */
        void urlEncode_f(const char * in, int len, std::string & output, char * buffer);

        inline void urlEncode_f(const std::string & input, std::string & output, char * buffer)
        {
            urlEncode_f(input.data(), input.length(), output, buffer);
        }

        /**
         * 快速url decode
         */
        void urlDecode_f(const char * in, int len, std::string & output, char * buffer);

        inline void urlDecode_f(const std::string & input, std::string & output, char * buffer)
        {
            urlDecode_f(input.data(), input.length(), output, buffer);
        }

        typedef std::map<adservice::types::string, adservice::types::string> ParamMap;

        /**
         * 从buffer中的url query中获取参数,其中url query应该是被url encode的,格式为xxx=xx&xx=xxx
         */
        void getParam(ParamMap & m, const char * buffer, char seperator = '&');

        void getParamv2(ParamMap & m, const char * buffer, char seperator = '&');

        void getParam(ParamMap & m, const adservice::types::string & input);

        void getParamv2(ParamMap & m, const adservice::types::string & input);

        /**
         * 从cookies中获取所有参数
         */
        void getCookiesParam(ParamMap & m, const char * buffer);

        /**
         * 从cookies串中提取目标参数
         */
        adservice::types::string extractCookiesParam(const adservice::types::string & key,
                                                     const adservice::types::string & input);

        /**
         * 从字符串中提取数字,input end with \0
         */
        long extractNumber(const char * input);

        /**
         * 从xxx-xxx-xxx形式的字符串提取国家,省,市
         */
        void extractAreaInfo(const char * input, int & country, int & province, int & city);

        bool parseHttpRequest(muduo::net::HttpRequest & request, const char * buffer, int size);

        /**
         * 提取不超过指定长度的字符串
         */
        inline std::string stringSafeInput(const std::string & input, std::string::size_type len)
        {
            if (input.length() > len) {
                return input.substr(0, len);
            }
            return input;
        }

        /**
         * 如果输入字符串超出指定长度,截断
         */
        inline void stringSafeInput(std::string & input, std::string::size_type len)
        {
            if (input.length() > len) {
                input = input.substr(0, len);
            }
        }

        /**
         * 提取没有其它冗余信息的合格的数字字符串
         */
        inline std::string stringSafeNumber(const std::string & input)
        {
            try {
                long num = std::stol(input);
                return std::to_string(num);
            } catch (std::exception & e) {
                return std::string("0");
            }
        }

        inline void stringSafeNumber(std::string & input)
        {
            const char * p = input.c_str();
            while (*p != '\0') {
                if (*p < 0x30 || *p > 0x39) {
                    input = "0";
                    return;
                }
                p++;
            }
        }

        bool url_replace(std::string & str, const std::string & from, const std::string & to);
    }
}
}

#endif // ADCORE_URL_H
