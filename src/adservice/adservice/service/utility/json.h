//
// Created by guoze.lin on 16/2/24.
//

#ifndef ADCORE_JSON_H
#define ADCORE_JSON_H

#include <cppcms/json.h>


namespace adservice {
namespace utility {

    namespace json {

        void tripslash2(char * str);

        bool parseJson(const char * json, cppcms::json::value & doc);

        bool parseJsonFile(const char * filePath, cppcms::json::value & doc);

        std::string toJson(const cppcms::json::value & doc);
    }
}
}

#endif // ADCORE_JSON_H
