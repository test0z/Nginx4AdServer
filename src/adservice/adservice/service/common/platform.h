//
// Created by guoze.lin on 16/1/22.
//

#ifndef ADCORE_PLATFORM_H
#define ADCORE_PLATFORM_H

#include "types.h"


namespace adservice{
    namespace platform{

        inline bool isLittleEndium(){
            union _what{
                uint8_t vs[4];
                int v;
            } w;
            w.v = 0X01020304;
            return w.vs[0] == 0X04;
        }
    }
}

#endif //ADCORE_PLATFORM_H
