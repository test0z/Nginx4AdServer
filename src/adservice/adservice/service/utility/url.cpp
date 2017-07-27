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

        std::string urlDecode_f(const std::string & input)
        {
            static UrlDecodeTable table;
            std::string output;
            output.resize(input.length() + 1);
            auto out = output.begin();
            const char * in = input.c_str();
            for (uint i = 0; i < input.length();) {
                if (in[i] == '%') {
                    *out += (table.map[(int)in[i + 1]] << 4) + table.map[(int)in[i + 2]];
                    i += 3;
                } else {
                    *out = in[i];
                    i++;
                }
                out++;
            }
            output.resize(out - output.begin());
            return output;
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

        std::string urlEncode_f(const std::string & input)
        {
            static UrlEncodeTable table;
            std::string output;
            output.resize(input.length() << 2);
            uint i = 0;
            auto out = output.begin();
            const char * in = input.c_str();
            while (i < input.length()) {
                if (table.entry[(int)in[i]]) {
                    *out++ = in[i++];
                } else {
                    *out++ = '%';
                    *out++ = table.map[(int)in[i]][0];
                    *out++ = table.map[(int)in[i]][1];
                    i++;
                }
            }
            *out = '\0';
            output.resize(out - output.begin());
            return output;
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

        void url_replace_all(std::string & str, const std::string & from, const std::string & to)
        {
            size_t start_pos = 0;
            do {
                start_pos = str.find(from, start_pos);
                if (start_pos == std::string::npos) {
                    break;
                } else {
                    str.replace(start_pos, from.length(), to);
                }
            } while (true);
        }

        std::string URLHelper::MACRO_HOLDER = "_mh_";
        std::string URLHelper::ENCODE_HOLDER = "_q_";

        int64_t URLParamMap::stringToInt(const std::string & s)
        {
            try {
                return std::stol(s);
            } catch (...) {
                return 0;
            }
        }

        std::string URLParamMap::intToString(int64_t i)
        {
            try {
                return std::to_string(i);
            } catch (...) {
                return "";
            }
        }

        std::vector<URLHelper::ParamBindingItem> URLHelper::paramBindingTable
            = { { URL_REFERER, PARAM_STRING },
                { URL_ADPLACE_ID, PARAM_STRING },
                { URL_MTTYADPLACE_ID, PARAM_STRING },
                { URL_EXPOSE_ID, PARAM_STRING },
                { URL_ADOWNER_ID, PARAM_INT },
                { URL_ADPLAN_ID, PARAM_INT },
                { URL_EXEC_ID, PARAM_INT },
                { URL_CREATIVE_ID, PARAM_INT },
                { URL_ADX_ID, PARAM_INT },
                { URL_CLICK_ID, PARAM_STRING },
                { URL_AREA_ID, PARAM_STRING },
                { URL_CLICK_X, PARAM_INT },
                { URL_CLICK_Y, PARAM_INT },
                { URL_ADX_MACRO, PARAM_STRING },
                { URL_BID_PRICE, PARAM_INT },
                { URL_EXCHANGE_PRICE, PARAM_STRING },
                { URL_MTTY_UID, PARAM_STRING },
                { URL_PRICE_TYPE, PARAM_INT },
                { URL_PRODUCTPACKAGE_ID, PARAM_INT },
                { URL_ORDER_ID, PARAM_INT },
                { URL_IMP_OF, PARAM_INT },
                { URL_DEAL_ID, PARAM_INT },
                { URL_DEVICE_IDFA, PARAM_STRING },
                { URL_DEVICE_IMEI, PARAM_STRING },
                { URL_DEVICE_ANDOROIDID, PARAM_STRING },
                { URL_DEVICE_MAC, PARAM_STRING },
                { URL_LANDING_URL, PARAM_STRING },
                { URL_TIMEM, PARAM_INT },
                { URL_SITE_ID, PARAM_INT },
                { URL_FLOWTYPE, PARAM_INT },
                { URL_IP, PARAM_STRING },
                { URL_ADX_BIDID, PARAM_STRING },
                { URL_ADX_IMPID, PARAM_STRING },
                { URL_CHANNEL, PARAM_INT },
                { URL_FEE_RATE, PARAM_STRING },
                { URL_FEE_RATE_STRS, PARAM_STRING },
                { URL_ADPLACE_BUY_TYPE, PARAM_INT } };

        void URLHelper::numberEncode(int64_t number, uchar_t *& buf, int & bufRemainSize)
        {
            if (bufRemainSize <= 10) {
                throw URLException("numberEncode:buffer reach boundary");
            }
            int i = 0;
            while (number > 0) {
                buf[i] = (number & 0x7F) | 0x80;
                number >>= 7;
                i++;
                bufRemainSize--;
            }
            buf[i - 1] &= 0x7F;
            buf = buf + i;
        }

        int64_t URLHelper::numberDecode(const uchar_t *& buf, const uchar_t * boundary)
        {
            int64_t result = 0;
            int limit = boundary - buf;
            int j = 0, i = 0;
            while (i < limit && buf[i] > 0x7F)
                i++;
            if (i >= limit) {
                throw URLException("numberDecode:reach boundary");
            }
            j = i;
            while (i > 0) {
                result += buf[i] & 0x7F;
                result <<= 7;
                i--;
            }
            result += buf[0] & 0x7F;
            buf += j + 1;
            return result;
        }

        void URLHelper::writeStringToBuf(const char * src, uchar_t *& dst, int size, int & bufRemainSize)
        {
            if (size > 0) {
                numberEncode(size, dst, bufRemainSize);
                if (bufRemainSize < size) {
                    throw URLException("buffer remained not large enough to hold string ");
                }
                memcpy((void *)dst, (const void *)src, size);
                dst += size;
                bufRemainSize -= size;
            } else if (bufRemainSize >= 1) {
                *dst++ = 0x00;
                bufRemainSize--;
            } else {
                throw URLException(
                    "writeStringToBuf:url encode invalid status,maybe size invalid or buffer not enough");
            }
        }

        void URLHelper::writeIntToBuf(int64_t src, uchar_t *& dst, int & bufRemainSize)
        {
            if (src > 0) {
                numberEncode(src, dst, bufRemainSize);
            } else if (bufRemainSize >= 1) {
                *dst++ = 0x00;
            } else {
                throw URLException("writeIntToBuf:url encode invalid status,maybe buffer not enough");
            }
        }

        std::string URLHelper::urlParamSerialize(ParamMap & paramMap)
        {
            uchar_t buf[2048];
            uchar_t * p = &buf[0];
            int bufRemain = sizeof(buf);
            uint64_t vc = 0xFFFFFFFF;
            for (auto iter : paramBindingTable) {
                auto fIter = paramMap.find(iter.first);
                if (fIter != paramMap.end()) {
                    const std::string & str = fIter->second;
                    encodeUrlParam(iter.second, str, p, bufRemain, vc);
                } else {
                    encodeUrlParam(iter.second, "", p, bufRemain, vc);
                }
            }
            std::stringstream ss;
            ss << std::hex << vc;
            macroParamMap[URLHelper::MACRO_HOLDER] = ss.str();
            return std::string((const char *)buf, (const char *)p);
        }

        uint64_t URLHelper::urlParamDeserialize(const std::string & input, ParamMap & paramMap)
        {
            const uchar_t * p = (const uchar_t *)input.c_str();
            const uchar_t * end = (const uchar_t *)input.c_str() + input.length();
            uint64_t vc = 0XFFFFFFFF;
            for (auto iter : paramBindingTable) {
                if (p == end) { // reach end but met with lately added field
                    continue;
                }
                if (iter.second == PARAM_INT) {
                    int64_t num = decodeUrlParamNum(p, end);
                    vc ^= (uint64_t)num;
                    if (num != 0) {
                        paramMap.insert({ iter.first, std::to_string(num) });
                    }
                } else if (iter.second == PARAM_STRING) {
                    std::string v = decodeUrlParamStr(p, end);
                    uint64_t len = v.length();
                    vc ^= len;
                    if (!v.empty()) {
                        paramMap.insert({ iter.first, v });
                    }
                }
            }
            return vc;
        }

        std::string URLHelper::cipherParam()
        {
            std::string encodeParam = urlParamSerialize(paramMap);
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
            return ENCODE_HOLDER + "=" + urlSafeParam;
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

        std::string URLHelper::toQuery()
        {
            std::string data = toUrlParam();
            std::string result;
            result += path.empty() ? "/" : path;
            result += data.empty() ? "" : std::string("?") + data;
            return result;
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

        std::string URLHelper::toUrlNoParam()
        {
            std::string result;
            result += protocolName.empty() ? "http://" : protocolName + "://";
            result += host.empty() ? "" : host;
            result += port.empty() ? "" : std::string(":") + port;
            result += path.empty() ? "" : path;
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
                    processParamDataWithRetry(std::string(cUrl + pos + 1, cUrl + url.length()), encoded);
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
            processParamDataWithRetry(data, encoded);
        }

        void URLHelper::processParamDataWithRetry(const std::string & data, bool encoded)
        {
            if (data.length() >= 2048) { //超长url
                LOG_ERROR << "url data too long,input:" << data;
                return;
            }
            if (!processParamData(data, encoded)) {
                try { //失败了一次，进行重试
                    std::string escapeData = data;
                    if (escapeData.rfind("-7c")
                        != std::string::npos) { //部分媒体会将url的％换成-导致无法decode,因此换回来再试一次
                        url_replace_all(escapeData, "-", "%");
                    }
                    char presetBuf[2048];
                    char * buf = &presetBuf[0];
                    std::string safeData;
                    urlDecode_f(escapeData, safeData, buf);
                    processParamData(safeData, encoded);
                } catch (std::exception & e) {
                    LOG_ERROR << "processParamDataWithRetry,retry failed,input:" << data;
                }
            }
        }

        bool URLHelper::processParamData(const std::string & data, bool encoded)
        {
            try {
                if (encoded) { // data是编码数据，解码
                    char presetBuf[2048];
                    char * buf = &presetBuf[0];
                    std::string safeData;
                    urlDecode_f(data, safeData, buf);
                    if (safeData.rfind("%7c")
                        != std::string::npos) { //部分媒体会将url做多次encode,因此若仍然包含％7c继续decode
                        std::string safeData2;
                        urlDecode_f(safeData, safeData2, buf);
                        safeData = safeData2;
                    }
                    std::vector<std::string> tmp;
                    boost::split(tmp, safeData, boost::is_any_of("|"));
                    if (tmp.size() > 0) {
                        std::string mainPart = tmp[0];
                        ParamMap cipherQueryMap;
                        getParamv2(cipherQueryMap, mainPart);
                        mainPart = cipherQueryMap[ENCODE_HOLDER];
                        std::string base64param;
                        if (!cypher::base64decode(mainPart, base64param)) {
                            return false;
                        }
                        uint64_t vc = urlParamDeserialize(base64param, this->paramMap);
                        for (uint i = 1; i < tmp.size() - 1; i += 2) {
                            paramMap.insert(std::make_pair(tmp[i], tmp[i + 1]));
                        }
                        std::string inputVc = paramMap.find(URLHelper::MACRO_HOLDER) != paramMap.end()
                                                  ? paramMap[URLHelper::MACRO_HOLDER]
                                                  : "";
                        std::stringstream ss;
                        ss << std::hex << vc;
                        std::string outputVc = ss.str();
                        if (inputVc != outputVc) { //检查校验码
                            LOG_ERROR << "verification code check error,inputVc:" << inputVc << ",got:" << outputVc;
                            this->paramMap.clear();
                        }
                    }
                } else {
                    getParamv2(this->paramMap, data);
                }
                return true;
            } catch (std::exception & e) {
                this->paramMap.clear();
                LOG_ERROR << "URLHelper::processParamData,exception:" << e.what() << ",data:" << data;
            }
            return false;
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

        void URLHelper::remove(const std::string & paramName)
        {
            auto iter = paramMap.find(paramName);
            if (iter != paramMap.end()) {
                paramMap.erase(iter);
            }
        }

        void URLHelper::removeMacro(const std::string & paramName)
        {
            auto iter = macroParamMap.find(paramName);
            if (iter != macroParamMap.end()) {
                macroParamMap.erase(iter);
            }
        }
    }
}
}
