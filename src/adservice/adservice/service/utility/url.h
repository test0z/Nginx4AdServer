//
// Created by guoze.lin on 16/2/24.
//

#ifndef ADCORE_URL_H
#define ADCORE_URL_H

#include "common/types.h"
#include <boost/serialization/access.hpp>
#include <cstring>
#include <ctime>
#include <exception>
#include <iostream>
#include <map>
#include <mtty/constants.h>
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

        void urlEncode_f(const std::string & input, std::string & output);

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

        void urlDecode_f(const std::string & input, std::string & output);

        typedef std::map<std::string, std::string> ParamMap;

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

        void url_replace_all(std::string & str, const std::string & from, const std::string & to);

        class URLParamMap {
        public:
            static int64_t stringToInt(const std::string & s);

            static std::string intToString(int64_t i);
        };

        class URLException : public std::exception {
        public:
            URLException()
            {
            }
            URLException(const std::string & msg)
                : message(msg)
            {
            }
            const char * what() const noexcept override
            {
                return message.c_str();
            }

        private:
            std::string message;
        };

        class URLHelper {
        public:
            typedef enum { PARAM_INT, PARAM_STRING } PARAM_TYPE;
            typedef std::pair<std::string, PARAM_TYPE> ParamBindingItem;
            static std::vector<ParamBindingItem> paramBindingTable;

        public:
            URLHelper()
            {
            }

            URLHelper(const std::string & url, bool encoded);

            URLHelper(const std::string & protocol, const std::string & host, const std::string & port,
                      const std::string & path);

            URLHelper(const std::string & protocol, const std::string & host, const std::string & port,
                      const std::string & path, const std::string & data, bool encoded);

            /**
             * 添加参数
             * @brief add
             * @param parmamName
             * @param paramValue
             */
            void add(const std::string & parmamName, const std::string & paramValue);

            /**
             * 删除参数
             * @brief remove
             * @param paramName
             */
            void remove(const std::string & paramName);

            /**
             * 不重复地添加参数，如果已存在返回false,否则返回true
             * @brief addNoDuplicate
             * @param paramName
             * @param paramValue
             * @return
             */
            bool addNoDuplicate(const std::string & paramName, const std::string & paramValue);

            /**
             * 添加宏
             * @brief addMacro
             * @param paramName
             * @param paramValue
             */
            void addMacro(const std::string & paramName, const std::string & paramValue);

            /**
             * 删除宏
             * @brief removeMacro
             * @param paramName
             */
            void removeMacro(const std::string & paramName);

            std::string getProtocol()
            {
                if (protocolName.empty()) {
                    return "http";
                } else
                    return protocolName;
            }

            void setProtocol(const std::string & protocol)
            {
                protocolName = protocol;
            }

            std::string getHost()
            {
                return host;
            }

            void setHost(const std::string & h)
            {
                host = h;
            }

            std::string getPort()
            {
                if (port.empty()) {
                    return "80";
                } else {
                    return port;
                }
            }

            void setPort(const std::string & p)
            {
                if (std::isdigit(*p.c_str())) {
                    port = p;
                }
            }

            std::string getPath()
            {
                return path;
            }

            void setPath(const std::string & p)
            {
                path = p;
            }

            ParamMap & getParamMap()
            {
                return paramMap;
            }

            /**
             * 生成加密链接
             * @brief cipherUrl
             * @return
             */
            std::string cipherUrl();

            /**
             * 生成加密参数
             * @brief cipherParam
             * @return
             */
            std::string cipherParam();

            /**
             * 从加密链接反向得到真实url
             * @brief fromCipherUrl
             * @return
             */
            static URLHelper fromCipherUrl(const std::string & cipher);

            /**
             * 转成url字符串
             * @brief toUrl
             * @return
             */
            std::string toUrl();

            /**
             * 转成url参数
             * @brief toUrlParam
             * @return
             */
            std::string toUrlParam();

            /**
             * 从url字符串反向解析
             * @brief fromUrl
             * @return
             */
            static URLHelper fromUrl(const std::string & url);

        private:
            void processParamDataWithRetry(const std::string & data, bool encoded);
            bool processParamData(const std::string & data, bool encoded);

        private:
            static void numberEncode(int64_t number, uchar_t *& buf, int & bufRemainSize);

            static int64_t numberDecode(const uchar_t *& buf, const uchar_t * boundary);

            static void writeStringToBuf(const char * src, uchar_t *& dst, int size, int & bufRemainSize);

            static void writeIntToBuf(int64_t src, uchar_t *& dst, int & bufRemainSize);

            std::string urlParamSerialize(ParamMap & paramMap);

            uint64_t urlParamDeserialize(const std::string & input, ParamMap & paramMap);

            static inline void encodeUrlParam(URLHelper::PARAM_TYPE & paramType, const std::string & value,
                                              uchar_t *& p, int & bufRemainSize, uint64_t & vc)
            {
                if (paramType == URLHelper::PARAM_STRING) {
                    URLHelper::writeStringToBuf(value.c_str(), p, value.length(), bufRemainSize);
                    vc ^= value.length();
                } else if (paramType == URLHelper::PARAM_INT) {
                    int64_t num = value.empty() ? 0 : URLParamMap::stringToInt(value);
                    URLHelper::writeIntToBuf(num, p, bufRemainSize);
                    vc ^= (uint64_t)num;
                }
            }

            static inline std::string decodeUrlParamStr(const uchar_t *& p, const uchar_t * boundary)
            {
                int64_t num = URLHelper::numberDecode(p, boundary);
                if (num > 0 && boundary >= p + num) {
                    const uchar_t * start = p;
                    p += num;
                    return std::string((const char *)start, (const char *)p);
                } else {
                    return "";
                }
            }

            static inline int64_t decodeUrlParamNum(const uchar_t *& p, const uchar_t * boundary)
            {
                int64_t num = URLHelper::numberDecode(p, boundary);
                return num;
            }

        public:
            static std::string MACRO_HOLDER;
            static std::string ENCODE_HOLDER;

        private:
            ParamMap paramMap;
            ParamMap macroParamMap;
            std::string protocolName;
            std::string host;
            std::string port;
            std::string path;
        };
    }
}
}

#endif // ADCORE_URL_H
