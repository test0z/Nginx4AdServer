//
// Created by guoze.lin on 16/2/24.
//

#ifndef ADCORE_HASH_H
#define ADCORE_HASH_H

#include "common/types.h"

namespace adservice{
    namespace utility{

        namespace hash{

            /** hash functions **/

            uint64_t sax_hash(const void *key, int32_t len);

            uint64_t fnv_hash(const void *key, int32_t len);

            uint64_t oat_hash(const void *key, int32_t len);
        }

    }
}

#endif //ADCORE_HASH_H
