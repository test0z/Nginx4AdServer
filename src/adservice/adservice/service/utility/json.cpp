//
// Created by guoze.lin on 16/2/2.
//

#include <exception>
#include <fstream>
#include <sstream>

#include "json.h"


namespace adservice {
namespace utility {
    namespace json {

        void tripslash2(char * str)
        {
            char * p1 = str;
            char * p2 = p1;
            while (*p2 != '\0') {
                if (*p2 == '\\' && p2[1] == '\"') {
                    p2++;
                }
                *p1++ = *p2++;
            }
            *p1 = '\0';
        }

        bool parseJson(const char * json, cppcms::json::value & doc)
        {
            std::stringstream ss;
            ss << json;
            ss >> doc;

            return ss;
        }

        bool parseJsonFile(const char * filePath, cppcms::json::value & doc)
        {
            std::ifstream fs(filePath, std::ios_base::in);
            if (!fs) {
                std::cerr << " can't open json file:" << filePath << std::endl;
                return false;
            }

            fs >> doc;

            return fs;
        }

        std::string toJson(const cppcms::json::value & doc)
        {
            std::stringstream ss;
            ss << doc;
            return ss.str();
        }
    }
}
}
