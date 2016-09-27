//
// Created by guoze.lin on 16/3/30.
//

#ifndef ADCORE_FILE_H
#define ADCORE_FILE_H

#include <string.h>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>

namespace adservice{
    namespace utility{
        namespace file{

            void loadFile(char* buffer,const char* filePath);

        }
    }
}

#endif //ADCORE_FILE_H
