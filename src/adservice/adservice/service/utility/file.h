//
// Created by guoze.lin on 16/3/30.
//

#ifndef ADCORE_FILE_H
#define ADCORE_FILE_H

#include <fstream>
#include <iostream>
#include <sstream>
#include <string.h>
#include <string>

namespace adservice {
namespace utility {
    namespace file {

        void loadFile(char * buffer, const char * filePath);

        std::string loadFile(const std::string & filePath);
    }
}
}

#endif // ADCORE_FILE_H
