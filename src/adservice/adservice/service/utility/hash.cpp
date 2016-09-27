//
// Created by guoze.lin on 16/1/20.
//

#include "utility.h"

namespace adservice{
    namespace utility{
        namespace hash{
            /**
             * [shift-and-xor hash method]
             * @param  key [input string]
             * @param  len [string length]
             * @return     [hash value]
             */
            uint64_t sax_hash(const void *key, int len) {
                const uchar_t *p = (const uchar_t*)key;
                uint64_t h = 0;
                int i;

                for (i = 0; i < len; i++) {
                    h ^= (h << 5) + (h >> 2) + p[i];
                }

                return h;
            }

            /**
             * [Fowler/Noll/Vo hash method]
             * see http://www.isthe.com/chongo/tech/comp/fnv/
             * @param  key [input string]
             * @param  len [string length]
             * @return     [hash value]
             */
            uint64_t fnv_hash(const void *key, int len) {
                const uchar_t *p = (const uchar_t*)key;
                uint64_t h = 14695981039346656037UL;
                int i;

                for (i = 0; i < len; i++) {
                    h = (h * 1099511628211) ^ p[i];
                }

                return h;
            }

            /**
             * [one-at-a-time hash method]
             * @param  key [input string]
             * @param  len [string length]
             * @return     [hash value]
             */
            uint64_t oat_hash(const void *key, int len) {
                const uchar_t *p = (const uchar_t*)key;
                uint64_t h = 0;
                int i;

                for (i = 0; i < len; i++) {
                    h += p[i];
                    h += (h << 10);
                    h ^= (h >> 6);
                }

                h += (h << 3);
                h ^= (h >> 11);
                h += (h << 15);

                return h;
            }
        }
    }
}


