#include "template_engine.h"
#include "file.h"
#include "logging.h"
#include <boost/regex.hpp>
#include <unordered_set>

namespace adservice {
namespace utility {

    namespace templateengine {

        namespace {
            bool someCharacter(char c)
            {
                if ((c >= 48 && c <= 57) || (c >= 65 && c <= 90) || (c >= 97 && c <= 122) || c == '_' || c == '.') {
                    return true;
                }
                return false;
            }

            bool inRegexpCharset(char c)
            {
                static std::unordered_set<char> charset{ '.', '{', '}', '[', ']', '(', ')', '\\', '|' };
                return charset.find(c) != charset.end();
            }
        }

        void TemplateEngine::compile(const std::string & templateFile)
        {
            templateFileContent = file::loadFile(templateFile);
            std::stringstream ss;
            const char *p = templateFileContent.c_str(), *begin = p;
            const char * paramStart = nullptr;
            while (*p != '\0') {
                if (paramStart != nullptr) {
                    if (*p == '}') {
                        std::string paramName = std::string(paramStart, p);
                        params.push_back(TemplateParam(paramStart - begin - 2, paramName));
                        paramStart = nullptr;
                        ss << "(\\s*.+\\s*)";
                    } else if (!someCharacter(*p)) {
                        ss << std::string(paramStart - 2, p + 1);
                        paramStart = nullptr;
                    }
                } else if (*p == '$' && *(p + 1) == '{') {
                    p += 2;
                    paramStart = p;
                    continue;
                } else if (paramStart == nullptr) {
                    if (inRegexpCharset(*p)) {
                        ss << "\\";
                    }
                    ss << *p;
                }
                p++;
            }
            templateRegexp = ss.str();
            if (paramStart != nullptr) {
                throw "TemplateEngine: there's no bracket ending the quote param at the end";
            }
        }

        std::string TemplateEngine::bind(const std::unordered_map<std::string, std::string> & paramMap)
        {
            std::stringstream ss;
            int pos = 0;
            const char * p = templateFileContent.c_str();
            int paramIdx = -1;
            TemplateParam currentPos(0x7FFFFFFF, "");
            if (params.size() > 0) {
                paramIdx = 0;
                currentPos.pos = params[0].pos;
                currentPos.param = params[0].param;
            }
            while (true) {
                while (pos < currentPos.pos && *p != '\0') {
                    ss << *p++;
                    pos++;
                }
                if (*p == '\0') {
                    break;
                }
                auto iter = paramMap.find(currentPos.param);
                if (iter != paramMap.end()) {
                    ss << iter->second;
                } else {
                    LOG_WARN << "param " << currentPos.param << " missing";
                }
                p += currentPos.param.length() + 3;
                pos += currentPos.param.length() + 3;
                if (paramIdx + 1 < params.size()) {
                    currentPos = params[++paramIdx];
                } else {
                    currentPos.pos = 0x7FFFFFFF;
                }
            }
            return ss.str();
        }

        void TemplateEngine::bindFast(const std::unordered_map<std::string, std::string> & paramMap,
                                      std::string & output)
        {
            int size = templateFileContent.length();
            for (auto iter : paramMap) {
                size += iter.second.length();
            }
            output.resize(size + 1);
            auto out = utput.begin();
            const char * p = templateFileContent.c_str();
            int pos = 0;
            int paramIdx = -1;
            TemplateParam currentPos(0x7FFFFFFF, "");
            if (params.size() > 0) {
                paramIdx = 0;
                currentPos.pos = params[0].pos;
                currentPos.param = params[0].param;
            }
            while (true) {
                while (pos < currentPos.pos && *p != '\0') {
                    *out++ = *p++;
                    pos++;
                }
                if (*p == '\0') {
                    break;
                }
                auto iter = paramMap.find(currentPos.param);
                if (iter != paramMap.end()) {
                    for (auto pb : iter->second) {
                        *out++ = pb;
                    }
                } else {
                    LOG_WARN << "param " << currentPos.param << " missing";
                }
                p += currentPos.param.length() + 3;
                pos += currentPos.param.length() + 3;
                if (paramIdx + 1 < params.size()) {
                    currentPos = params[++paramIdx];
                } else {
                    currentPos.pos = 0x7FFFFFFF;
                }
            }
        }

        std::unordered_map<std::string, std::string> TemplateEngine::getVariables(const std::string & shadowFile)
        {
            std::unordered_map<std::string, std::string> result;
            const std::string & shadowFileContent = file::loadFile(shadowFile);
            boost::regex regexpression(templateRegexp);
            boost::smatch match;
            if (boost::regex_search(shadowFileContent, match, regexpression)) {
                if (match.size() != params.size() + 1) {
                    LOG_ERROR << "shadow file not match template,please check";
                    return result;
                }
                for (uint32_t i = 0; i < match.size(); i++) {
                    auto & p = params[i];
                    result.insert({ p.param, match[i] });
                }
            } else {
                throw "shadowFile not match template";
            }
            return result;
        }
    }
}
}
