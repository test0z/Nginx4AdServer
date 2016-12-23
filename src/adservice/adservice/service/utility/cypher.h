//
// Created by guoze.lin on 16/2/24.
//

#ifndef ADCORE_CYPHER_H
#define ADCORE_CYPHER_H

#include "common/types.h"
#include "mttytime.h"
#include <array>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <stddef.h>
#include <strings.h>
#include <tuple>
#include <vector>

namespace adservice {
namespace utility {

    namespace cypher {

        char_t * toHex(const uchar_t * input, int32_t size, INOUT char_t * hexResult);

        char_t * toHexReadable(const uchar_t * input, int32_t size, INOUT char_t * hexResult);

        char_t * hexToReadable(const char_t * hexString, int32_t size, INOUT char_t * readableResult);

        uchar_t * fromHex(const char_t * hexInput, int32_t size, INOUT uchar_t * result);

        uchar_t * fromLittleEndiumHex(const char * hexInput, int32_t size, INOUT uchar_t * result,
                                      INOUT int32_t & outSize, bool capital = true);

        void urlsafe_base64decode(std::string input, std::string & output);

        void aes_ecbencode(const uchar_t * key, const std::string & input, std::string & output);

        void aes_ecbdecode(const uchar_t * key, const std::string & input, std::string & output);

        void aes_ecbdecode_nopadding(const uchar_t * key, const std::string & input, std::string & output);

        std::string md5_encode(const std::string & input);

        void base64encode(const std::string & input, std::string & output);

        bool base64decode(const std::string & input, std::string & output);

        std::string encodePrice(int price, bool useAes = false);

        std::string randomId(int field);

        /**
         * 用于调试输入输出,解析十六进制字符流
         */
        class HexResolver final {
        public:
            explicit HexResolver(int stringSize)
            {
                resultSize = (stringSize >> 1) + 2;
                result = new uchar_t[resultSize];
            }

            void resolve(const char_t * input, int size)
            {
                fromLittleEndiumHex(input, size, result, resultSize);
            }

            typedef void (*SHOWHANDLER)(uchar_t *, int32_t);

            void show(SHOWHANDLER handler = nullptr)
            {
                if (handler != nullptr)
                    handler(result, resultSize);
                else
                    printResult();
            }

            ~HexResolver()
            {
                if (result != nullptr)
                    delete[] result;
            }

        private:
            void printResult()
            {
                result[resultSize] = '\0';
                printf("%s", result);
            }

            uchar_t * result;
            int resultSize;
        };

        /**
          * 密码表生成器,用于离线计算密码表
          */
        class CypherMapGenerator {
        public:
            typedef const char (*CypherMapArrayPointer)[16];
            typedef const int (*CypherMapIndexArrayPointer)[16];

            /**
             * 构造器
             * @param isInit:是否需要自动初始化,自动初始化将自动生成结果
             */
            CypherMapGenerator(bool isInit);

            /**
             * 使用给定字符集产生一个乱码表
             */
            std::array<char, 64> randomCharSequence();

            /**
             * 生成码表的索引
             */
            void regenerate();

            /**
             * 输出结果
             */
            void print();

            inline void setCypherMap(CypherMapArrayPointer array)
            {
                for (int i = 0; i < 4; i++) {
                    for (int j = 0; j < 16; j++) {
                        cypherMap[i][j] = array[i][j];
                    }
                }
            }

            inline CypherMapArrayPointer getCypherMap() const
            {
                return (CypherMapArrayPointer)cypherMap;
            }

            inline CypherMapArrayPointer getCypherSortMap() const
            {
                return (CypherMapArrayPointer)cypherSortMap;
            }

            inline CypherMapIndexArrayPointer getCypherPosMap() const
            {
                return (CypherMapIndexArrayPointer)cypherPosMap;
            }

            class InnerComparetor {
            public:
                InnerComparetor(CypherMapGenerator * g)
                    : generator(g)
                {
                }

                bool operator()(const int & a, const int & b)
                {
                    return generator->currentCypher[a] < generator->currentCypher[b];
                }

            private:
                CypherMapGenerator * generator;
            };

        private:
            char cypherMap[4][16];
            char cypherSortMap[4][16];
            int cypherPosMap[4][16];
            int indexMap[4][16];
            char * currentCypher;
            int * current;
        };

        typedef union {
            char_t char_bytes[20];
            uchar_t bytes[16];
            int32_t words[4];
            int64_t dwords[2];
        } CypherResult128;

        typedef union {
            uchar_t bytes[8];
            int32_t words[2];
            int64_t dword;
        } DecodeResult64;

        /**
         * 将给定字符串进行加密,字符串不超过8字节
         * @param input
         * @param size: input的大小
         * @param result:用于存放结果
         */
        void cookiesEncode(const uchar_t * input, const int32_t size, INOUT CypherResult128 & result);

        /**
         * 将给定加密字符串进行解密
         * @param input
         * @param output : 输出结果
         * @param size: 存放结果大小
         */
        bool cookiesDecode(const CypherResult128 & input, INOUT uchar_t * output, INOUT int32_t & size);

        /**
         * 将给定加密字符串进行解密
         * @param input
         * @param output : 输出结果
         */
        inline bool cookiesDecode(const CypherResult128 & input, INOUT DecodeResult64 & output)
        {
            int size = 8;
            return cookiesDecode(input, output.bytes, size);
        }

        /**
         * 生成一个明文cookies串
         * @param result:用于存放结果
         * @param size:进入时指定result缓存的大小,返回时指出结果的长度,以字节为单位
         */
        void makeCookiesPublic(INOUT char_t * result, INOUT int32_t & size);

        /**
         * 生成一个加密的cookies串
         * @param result:用于存放结果
         */
        void makeCookies(INOUT CypherResult128 & result);
    }
}
}

#endif // ADCORE_CYPHER_H
