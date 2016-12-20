//
// Created by guoze.lin on 16/2/19.
//

#include "url.h"
#include "common/functions.h"
#include "logging.h"
#include <boost/algorithm/string.hpp>
#include <boost/serialization/map.hpp>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mtty/utils.h>

namespace adservice {
namespace utility {
    namespace url {

        std::string urlEncode(const std::string & input)
        {
            std::ostringstream out;
            const char * in = input.c_str();
            for (std::string::size_type i = 0; i < input.length(); ++i) {
                char t = in[i];
                if (t == '-' || (t >= '0' && t <= '9') || (t >= 'A' && t <= 'Z') || t == '_' || (t >= 'a' && t <= 'z')
                    || t == '~') {
                    out << t;
                } else {
                    out << '%' << std::setw(2) << std::setfill('0') << std::hex << (int)in[i];
                }
            }
            return out.str();
        }

        std::string urlDecode(const std::string & input)
        {
            std::ostringstream out;
            const char * in = input.c_str();
            for (std::string::size_type i = 0; i < input.length(); ++i) {
                if (in[i] == '%') {
                    char oc = 0;
                    char c = in[i + 1];
                    if (c >= 'a') {
                        c = c - 'a' + 10;
                    } else if (c >= 'A') {
                        c = c - 'A' + 10;
                    } else {
                        c -= '0';
                    }
                    oc = c << 4;
                    c = in[i + 2];
                    if (c >= 'a') {
                        c = c - 'a' + 10;
                    } else if (c >= 'A') {
                        c = c - 'A' + 10;
                    } else {
                        c -= '0';
                    }
                    oc += c;
                    out << oc;
                    i += 2;
                } else {
                    out << in[i];
                }
            }
            return out.str();
        }

        struct UrlDecodeTable {
            char map[128];
            UrlDecodeTable()
            {
                for (int i = 0; i < 128; i++) {
                    if (i >= 'a') {
                        map[i] = i - 'a' + 10;
                    } else if (i >= 'A') {
                        map[i] = i - 'A' + 10;
                    } else {
                        map[i] = i - '0';
                    }
                }
            }
        };

        struct UrlEncodeTable {
            bool entry[128];
            char map[128][2];
            UrlEncodeTable()
            {
                char codetable[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
                for (int i = 0; i < 128; i++) {
                    if ((i >= '0' && i <= '9') || (i >= 'A' && i <= 'Z') || (i >= 'a' && i <= 'z') || i == '-'
                        || i == '_'
                        || i == '~') {
                        entry[i] = true;
                    } else {
                        entry[i] = false;
                        map[i][0] = codetable[(i >> 4) & 0x0F];
                        map[i][1] = codetable[i & 0x0F];
                    }
                }
            }
        };

        void urlDecode_f(const char * in, int len, adservice::types::string & output, char * buffer)
        {
            static UrlDecodeTable table;
            memset(buffer, 0, len + 1);
            int i = 0, j = 0;
            for (i = 0, j = 0; i < len;) {
                if (in[i] == '%') {
                    buffer[j] += (table.map[(int)in[i + 1]] << 4) + table.map[(int)in[i + 2]];
                    i += 3;
                } else {
                    buffer[j] = in[i];
                    i++;
                }
                j++;
            }
            output.assign(buffer, buffer + j);
        }

        void urlEncode_f(const char * in, int len, std::string & output, char * buffer)
        {
            static UrlEncodeTable table;
            int i = 0, j = 0;
            while (i < len) {
                if (table.entry[(int)in[i]]) {
                    buffer[j++] = in[i++];
                } else {
                    buffer[j] = '%';
                    buffer[j + 1] = table.map[(int)in[i]][0];
                    buffer[j + 2] = table.map[(int)in[i]][1];
                    j += 3;
                    i++;
                }
            }
            buffer[j] = '\0';
            output.assign(buffer, buffer + j);
        }

        void getParam(ParamMap & m, const char * buffer, char seperator)
        {
            const char *p = buffer, *p0 = buffer;
            adservice::types::string key, value;
            while (*p != '\0') {
                if (*p == '=') {
                    while (*p0 == ' ')
                        p0++;
                    if (p0 != p)
                        key.assign(p0, p);
                    p0 = p + 1;
                } else if (*p == seperator || *p == '\0') {
                    while (*p0 == ' ')
                        p0++;
                    if (p0 != p) {
                        value.assign(p0, p);
                        m[key] = value;
                    }
                    p0 = p + 1;
                }
                p++;
            }
            if (*p == '\0' && p != p0) {
                value.assign(p0, p);
                m[key] = value;
            }
        }

        void getParamv2(ParamMap & m, const char * buffer, char seperator)
        {
            const char *p = buffer, *p0 = buffer;
            bool pairFlag = false;
            adservice::types::string key, value;
            while (*p != '\0') {
                if (!pairFlag && *p == '=') {
                    while (*p0 == ' ')
                        p0++;
                    if (p0 != p) {
                        key.assign(p0, p);
                        pairFlag = true;
                    }
                    p0 = p + 1;
                } else if (*p == seperator || *p == '\0') {
                    while (*p0 == ' ')
                        p0++;
                    if (p0 != p || *p0 == seperator) {
                        value.assign(p0, p);
                        m[key] = value;
                        pairFlag = false;
                    }
                    p0 = p + 1;
                }
                p++;
            }
            if (*p == '\0' && p != p0) {
                value.assign(p0, p);
                m[key] = value;
            }
        }

        void getParam(ParamMap & m, const adservice::types::string & input)
        {
            getParam(m, input.c_str());
        }

        void getParamv2(ParamMap & m, const adservice::types::string & input)
        {
            getParamv2(m, input.c_str());
        }

        void getCookiesParam(ParamMap & m, const char * buffer)
        {
            getParam(m, buffer, ';');
        }

        adservice::types::string extractCookiesParam(const adservice::types::string & key,
                                                     const adservice::types::string & input)
        {
            ParamMap m;
            getParamv2(m, input.c_str(), ';');
            return m[key];
        }

        long extractNumber(const char * input)
        {
            long result = 0;
            const char * p = input;
            while (*p != '\0') {
                if (*p >= 0x30 && *p <= 0x39) {
                    result = (result << 3) + (result << 1) + (*p - 0x30); // result*=10; which is faster?need to profile
                    // result = result*10 + (*p - 0x30);
                }
                p++;
            }
            return result;
        }

        void extractAreaInfo(const char * input, int & country, int & province, int & city)
        {
            const char *p1 = input, *p2 = input;
            while (*p2 != '\0' && *p2 != '-')
                p2++;
            country = atoi(p1);
            p1 = ++p2;
            while (*p2 != '\0' && *p2 != '-')
                p2++;
            province = atoi(p1);
            p1 = ++p2;
            while (*p2 != '\0' && *p2 != '-')
                p2++;
            city = atoi(p1);
        }

        bool url_replace(std::string & str, const std::string & from, const std::string & to)
        {
            size_t start_pos = str.find(from);
            if (start_pos == std::string::npos)
                return false;
            str.replace(start_pos, from.length(), to);
            return true;
        }

        std::string URLHelper::MACRO_HOLDER = "_mh_";
        std::string URLHelper::ENCODE_HOLDER = "_q_";

        std::string URLHelper::cipherParam()
        {
            std::string encodeParam = MT::common::bserialize(paramMap);
            std::string base64Param;
            cypher::base64encode(encodeParam, base64Param);
            char presetBuf[2048];
            char * buf = &presetBuf[0];
            std::string urlSafeParam;
            urlEncode_f(base64Param, urlSafeParam, buf);
            for (auto & iter : macroParamMap) {
                urlSafeParam += "%7c";
                urlSafeParam += iter.first + "%7c";
                urlSafeParam += iter.second;
            }
            urlSafeParam += "%7c";
            return urlSafeParam;
        }

        std::string URLHelper::cipherUrl()
        {
            try {
                std::string urlSafeParam = cipherParam();
                std::string result;
                result += protocolName.empty() ? "http://" : protocolName + "://";
                result += host.empty() ? "" : host;
                result += port.empty() ? "" : std::string(":") + port;
                result += path.empty() ? "" : path;
                result += urlSafeParam.empty() ? "" : std::string("?") + urlSafeParam;
                return result;
            } catch (std::exception & e) {
                LOG_ERROR << "URLHelper::cipherUrl,exception:" << e.what();
            }
            return "";
        }

        URLHelper URLHelper::fromCipherUrl(const std::string & cipherUrl)
        {
            return URLHelper(cipherUrl, true);
        }

        std::string URLHelper::toUrlParam()
        {
            std::string data;
            if (!paramMap.empty()) {
                int seq = 0;
                for (auto & iter : paramMap) {
                    if (seq++ != 0) {
                        data += "&";
                    }
                    data += iter.first + "=" + iter.second;
                }
            }
            if (!macroParamMap.empty()) {
                int seq = data.empty() ? 0 : 1;
                for (auto & iter : macroParamMap) {
                    if (seq++ != 0) {
                        data += "&";
                    }
                    data += iter.first + "=" + iter.second;
                }
            }
            return data;
        }

        std::string URLHelper::toUrl()
        {
            std::string data = toUrlParam();
            std::string result;
            result += protocolName.empty() ? "http://" : protocolName + "://";
            result += host.empty() ? "" : host;
            result += port.empty() ? "" : std::string(":") + port;
            result += path.empty() ? "" : path;
            result += data.empty() ? "" : std::string("?") + data;
            return result;
        }

        URLHelper URLHelper::fromUrl(const std::string & url)
        {
            return URLHelper(url, false);
        }

        URLHelper::URLHelper(const std::string & url, bool encoded)
        {
            std::string::size_type pos = 0;
            const char * cUrl = url.c_str();
            const char * hostBegin = url.c_str();
            while ((pos = url.find(":", pos)) != std::string::npos) {
                if (pos < url.length() - 3 && cUrl[pos + 1] == '/' && cUrl[pos + 2] == '/') { // protocol
                    protocolName = std::string(cUrl, cUrl + pos);
                    hostBegin = cUrl + pos + 3;
                    pos += 3;
                } else if (pos < url.length() - 2 && std::isdigit(cUrl[pos + 1])) { // host and port
                    host = std::string(hostBegin, cUrl + pos);
                    int32_t p = atoi(cUrl + pos + 1);
                    port = std::to_string(p);
                    break;
                } else {
                    break;
                }
            }
            pos = hostBegin - cUrl;
            if ((pos = url.find("/", pos)) != std::string::npos) {
                if (host.empty()) {
                    host = std::string(hostBegin, cUrl + pos);
                }
                const char * pathBegin = cUrl + pos;
                if ((pos = url.find("?", pos)) != std::string::npos) {
                    path = std::string(pathBegin, cUrl + pos);
                    processParamData(std::string(cUrl + pos + 1, cUrl + url.length()), encoded);
                    return;
                } else {
                    path = std::string(pathBegin, cUrl + url.length());
                }
            }
        }

        URLHelper::URLHelper(const std::string & protocol, const std::string & host, const std::string & port,
                             const std::string & path)
        {
            this->protocolName = protocol;
            this->host = host;
            this->port = port;
            this->path = path;
            if (*this->path.begin() != '/') {
                this->path = std::string("/") + this->path;
            }
        }

        URLHelper::URLHelper(const std::string & protocol, const std::string & host, const std::string & port,
                             const std::string & path, const std::string & data, bool encoded)
        {
            this->protocolName = protocol;
            this->host = host;
            this->port = port;
            this->path = path;
            if (*this->path.begin() != '/') {
                this->path = std::string("/") + this->path;
            }
            processParamData(data, encoded);
        }

        void URLHelper::processParamData(const std::string & data, bool encoded)
        {
            try {
                if (encoded) { // data是编码数据，解码
                    char presetBuf[2048];
                    char * buf = &presetBuf[0];
                    std::string safeData;
                    urlDecode_f(data, safeData, buf);
                    std::vector<std::string> tmp;
                    boost::split(tmp, safeData, boost::is_any_of("|"));
                    if (tmp.size() > 0) {
                        std::string mainPart = tmp[0];
                        std::string::size_type pos;
                        if ((pos = mainPart.rfind("&", mainPart.length() - 1)) != std::string::npos) {
                            mainPart = std::string(mainPart.begin() + pos + 1, mainPart.end());
                        }
                        std::string base64param;
                        cypher::base64decode(mainPart, base64param);
                        MT::common::bdeserialize(base64param, this->paramMap);
                        for (uint i = 1; i < tmp.size() - 1; i += 2) {
                            paramMap.insert(std::make_pair(tmp[i], tmp[i + 1]));
                        }
                    }
                } else {
                    getParamv2(this->paramMap, data);
                }
                // output paramMap
            } catch (std::exception & e) {
                LOG_ERROR << "URLHelper::processParamData,exception:" << e.what() << ",data:" << data;
            }
        }

        void URLHelper::add(const std::string & paramName, const std::string & paramValue)
        {
            paramMap[paramName] = paramValue;
        }

        bool URLHelper::addNoDuplicate(const std::string & paramName, const std::string & paramValue)
        {
            return paramMap.insert(std::make_pair(paramName, paramValue)).second;
        }

        void URLHelper::addMacro(const std::string & paramName, const std::string & paramValue)
        {
            macroParamMap[paramName] = paramValue;
        }
    }
}
}
