//
// Created by guoze.lin on 16/1/22.
//

#include "cypher.h"
#include "common/functions.h"
#include "logging.h"
#include <algorithm>
#include <array>
#include <cryptopp/aes.h>
#include <cryptopp/base64.h>
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/hex.h>
#include <cryptopp/md5.h>
#include <cryptopp/modes.h>
#include <mtty/constants.h>
#include <random>

namespace adservice {
namespace utility {

    namespace rng {
        extern int32_t randomInt();

        extern double randomDouble();
    }

    namespace cypher {

        using namespace std;

        //以下方法均基于小端

        /**
         * toHex 将二进制串转换成十六进制字符串
         * input : 输入串
         * size  : 串大小
         * hexResult :
         * 输入的用于存放结果的十六进制字符串,输出的结果为低地址低字节,输入串字节的低4位的编码结果前于高4位的编码结果
         * @return : 返回结果的首地址
         */
        char_t * toHex(const uchar_t * input, int32_t size, INOUT char * hexResult)
        {
            static char hexMap[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
            for (int i = 0, j = 0; i < size; i++, j += 2) {
                hexResult[j] = hexMap[input[i] & 0x0F];
                hexResult[j + 1] = hexMap[input[i] >> 4];
            }
            hexResult[size << 1] = '\0';
            return hexResult;
        }

        char_t * toHex(bool isCapital, const uchar_t * input, int32_t size, INOUT char * hexResult)
        {
            if (isCapital)
                return toHex(input, size, hexResult);
            static char hexMap[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
            for (int i = 0, j = 0; i < size; i++, j += 2) {
                hexResult[j] = hexMap[input[i] & 0x0F];
                hexResult[j + 1] = hexMap[input[i] >> 4];
            }
            hexResult[size << 1] = '\0';
            return hexResult;
        }

        /**
         * 因为toHex的结果是低地址低字节的,所以不符合人类阅读习惯
         * 使用toHexReadable将得到可读的十六进制字符串
         */
        char_t * toHexReadable(const uchar_t * input, int32_t size, INOUT char * hexResult)
        {
            toHex(input, size, hexResult);
            uint16_t * _hexResult = (uint16_t *)hexResult;
            for (int i = 0, j = size - 1; i < j; i++, j--) {
                _hexResult[i] ^= _hexResult[j];
                _hexResult[j] ^= _hexResult[i];
                _hexResult[i] ^= _hexResult[j];
            }
            for (int i = (size << 1) - 1; i > 0; i -= 2) {
                hexResult[i] ^= hexResult[i - 1];
                hexResult[i - 1] ^= hexResult[i];
                hexResult[i] ^= hexResult[i - 1];
            }
            return hexResult;
        }

        /**
         * 将toHex产生的结果转为可读字符串
         */
        char_t * hexToReadable(const char_t * hexString, int32_t size, INOUT char_t * readableResult)
        {
            uint16_t * in = (uint16_t *)hexString;
            uint16_t * out = (uint16_t *)readableResult;
            size >>= 1;
            for (int i = 0, j = size - 1; i < size && j >= 0; i++, j--) {
                out[j] = in[i];
            }
            size <<= 1;
            for (int i = size - 1; i > 0; i -= 2) {
                readableResult[i] ^= readableResult[i - 1];
                readableResult[i - 1] ^= readableResult[i];
                readableResult[i] ^= readableResult[i - 1];
            }
            readableResult[size] = '\0';
            return readableResult;
        }

        /**
         * fromHex: 从十六进制串得到二进制串
         * hexInput : 十六进制字符串,从toHex得到的结果
         * size : 串大小
         * result : 输入的用于存放结果的二进制串
         * @return : 返回结果的首地址
         */
        uchar_t * fromHex(const char * hexInput, int32_t size, INOUT uchar_t * result)
        {
            static uchar_t mask[2] = { 0x0F, 0xF0 };
            for (int i = 0; i < size; i++) {
                int j = i >> 1;
                if (!(i & 0x01))
                    result[j] = 0;
                if (hexInput[i] < 'A') {
                    result[j] |= ((hexInput[i] - '0') << ((i & 0x01) ? 4 : 0)) & mask[i & 0x01];
                } else {
                    result[j] |= ((hexInput[i] - 'A' + 10) << ((i & 0x01) ? 4 : 0)) & mask[i & 0x01];
                }
            }
            return result;
        }

        /**
         * 从符合小端格式的十六进制字符串中得到其机器表示
         * hexInput : 十六进制字符串,符合小端格式,比如使用tcpdump -x得到的十六进制串
         * size : 串大小
         * result : 输入的用于存放结果的二进制串
         * outSize: 表明输出串的大小,以字节为单位
         * capital: 输入结果是否大写
         * @return : 返回结果的首地址
         */
        uchar_t * fromLittleEndiumHex(const char * hexInput, int32_t size, INOUT uchar_t * result,
                                      INOUT int32_t & outSize, bool capital)
        {
            int needSize = (size & 0x01) ? (size >> 1) + 1 : (size >> 1);
            assert(outSize >= needSize);
            outSize = needSize;
            char base = capital ? 'A' : 'a';
            static uchar_t mask[2] = { 0xF0, 0x0F };
            for (int i = 0; i < size; i++) {
                int j = i >> 1;
                if (!(i & 0x01))
                    result[j] = 0;
                if (hexInput[i] < base) {
                    result[j] |= ((hexInput[i] - '0') << ((i & 0x01) ? 0 : 4)) & mask[i & 0x01];
                } else {
                    result[j] |= ((hexInput[i] - base + 10) << ((i & 0x01) ? 0 : 4)) & mask[i & 0x01];
                }
            }
            return result;
        }

        void urlsafe_base64decode(std::string input, std::string & output)
        {
            try {
                for (auto it = input.begin(); it != input.end(); ++it) {
                    if (*it == '-') {
                        *it = '+';
                    } else if (*it == '_') {
                        *it = '/';
                    }
                }
                CryptoPP::Base64Decoder decoder;
                decoder.Attach(new CryptoPP::StringSink(output));
                decoder.Put((uchar_t *)input.c_str(), input.size());
                decoder.MessageEnd();
            } catch (exception & e) {
                LOG_ERROR << "urlsafe_base64decode failed,e:" << e.what();
            }
        }

        void base64encode(const std::string & input, std::string & output)
        {
            try {
                CryptoPP::Base64Encoder encoder;
                encoder.Attach(new CryptoPP::StringSink(output));
                encoder.Put((const uchar_t *)input.c_str(), input.size());
                encoder.MessageEnd();
            } catch (exception & e) {
                LOG_ERROR << "base64encode failed,e:" << e.what();
            }
        }

        bool base64decode(const std::string & input, std::string & output)
        {
            try {
                CryptoPP::Base64Decoder encoder;
                encoder.Attach(new CryptoPP::StringSink(output));
                encoder.Put((const uchar_t *)input.c_str(), input.size());
                encoder.MessageEnd();
            } catch (exception & e) {
                LOG_ERROR << "base64encode failed,e:" << e.what();
                return false;
            }
            return true;
        }

        void aes_ecbencode(const uchar_t * key, const std::string & input, std::string & output, int keyLen)
        {
            try {
                CryptoPP::ECB_Mode<CryptoPP::AES>::Encryption e;
                e.SetKey(key, keyLen);
                CryptoPP::StringSource(input, true,
                                       new CryptoPP::StreamTransformationFilter(e, new CryptoPP::StringSink(output)));
            } catch (CryptoPP::Exception & e) {
                LOG_ERROR << "aes_ecbencode failed,e:" << e.what();
            }
        }

        void aes_ecbdecode(const uchar_t * key, const std::string & input, std::string & output, int keyLen)
        {
            try {
                CryptoPP::ECB_Mode<CryptoPP::AES>::Decryption d;
                d.SetKey(key, keyLen);
                CryptoPP::StringSource(input, true,
                                       new CryptoPP::StreamTransformationFilter(d, new CryptoPP::StringSink(output)));
            } catch (CryptoPP::Exception & e) {
                LOG_ERROR << "aes_ecbdecode failed,e:" << e.what();
            }
        }

        void aes_ecbdecode_nopadding(const uchar_t * key, const std::string & input, std::string & output, int keyLen)
        {
            try {
                CryptoPP::ECB_Mode<CryptoPP::AES>::Decryption d;
                d.SetKey(key, keyLen);
                CryptoPP::StringSource(
                    input, true, new CryptoPP::StreamTransformationFilter(d, new CryptoPP::StringSink(output),
                                                                          CryptoPP::BlockPaddingSchemeDef::NO_PADDING));
            } catch (CryptoPP::Exception & e) {
                LOG_ERROR << "aes_ecbdecode failed,e:" << e.what();
            }
        }

        void aes_cfbdecode(const uchar_t * key, const uchar_t * iv, const std::string & input, std::string & output)
        {
            try {
                CryptoPP::CFB_Mode<CryptoPP::AES>::Decryption d(key, 16, iv);
                CryptoPP::StringSource(input, true,
                                       new CryptoPP::StreamTransformationFilter(d, new CryptoPP::StringSink(output)));
            } catch (CryptoPP::Exception & e) {
                LOG_ERROR << "aes_cfbdecode failed,e:" << e.what();
            }
        }

        void aes_cbcdecode(const uchar_t * key, const uchar_t * iv, const std::string & input, std::string & output)
        {
            try {
                CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption d;
                d.SetKeyWithIV(key, 16, iv, 16);
                CryptoPP::StringSource(input, true, new CryptoPP::StreamTransformationFilter(
                                                        d, new CryptoPP::StringSink(output),
                                                        CryptoPP::BlockPaddingSchemeDef::PKCS_PADDING));
            } catch (CryptoPP::Exception & e) {
                LOG_ERROR << "aes_cfbdecode failed,e:" << e.what();
            }
        }

        std::string md5_encode(const std::string & input)
        {
            if (input.empty()) {
                return input;
            }
            try {
                CryptoPP::Weak::MD5 md5;
                byte digest[CryptoPP::Weak::MD5::DIGESTSIZE];
                md5.CalculateDigest(digest, (const byte *)input.c_str(), input.length());
                CryptoPP::HexEncoder encoder;
                std::string output;
                encoder.Attach(new CryptoPP::StringSink(output));
                encoder.Put(digest, sizeof(digest));
                encoder.MessageEnd();
                return output;
            } catch (CryptoPP::Exception & e) {
                LOG_ERROR << "md5_encode failed,e:" << e.what();
            }
            return input;
        }

        std::string encodePrice(int price, bool useAes)
        {
            if (!useAes) {
                return std::to_string(price);
            } else {
                std::string result;
                aes_ecbencode((const unsigned char *)OFFERPRICE_ENCODE_KEY, std::to_string(price), result);
                return result;
            }
        }

        CypherMapGenerator::CypherMapGenerator(bool isInit)
        {
            if (!isInit) {
                return;
            }
            array<char, 64> randomString = randomCharSequence();
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 16; j++) {
                    cypherMap[i][j] = randomString[(i << 4) + j];
                }
            }
            regenerate();
        }

        std::string randomId(int field)
        {
            char buffer[256];
            char * p = buffer;
            for (int i = 0; i < field; i++) {
                int r = rng::randomInt();
                toHex(false, (const uchar_t *)&r, sizeof(int), p);
                p += sizeof(int) << 1;
            }
            if (p != buffer)
                *(p - 1) = '\0';
            else
                *p = '\0';
            return std::string(buffer);
        }

        /**
         * randomCharSequence: 生成固定字符集的随机字符顺序
         * @return : 结果
         */
        array<char, 64> CypherMapGenerator::randomCharSequence()
        {
            array<char, 64> keys = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
                                     'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'M', 'E',
                                     'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U',
                                     'V', 'X', 'Y', 'Z', '=', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '_' };
            std::shuffle(keys.begin(), keys.end(), std::random_device());
            return keys;
        }

        /**
         * 生成密码表索引
         */
        void CypherMapGenerator::regenerate()
        {
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 16; j++) {
                    indexMap[i][j] = j;
                }
            }
            InnerComparetor comp(this);
            for (int i = 0; i < 4; i++) {
                current = indexMap[i];
                currentCypher = cypherMap[i];
                std::sort(current, current + 16, comp);
            }
            for (int i = 0; i < 4; i++) {
                currentCypher = cypherMap[i];
                current = indexMap[i];
                for (int j = 0; j < 16; j++) {
                    cypherSortMap[i][j] = currentCypher[current[j]];
                }
            }
            for (int i = 0; i < 4; i++) {
                currentCypher = cypherMap[i];
                current = indexMap[i];
                for (int j = 0; j < 16; j++) {
                    cypherPosMap[i][j] = current[j];
                }
            }
        }

        void CypherMapGenerator::print()
        {
            printf("cypherMap:{");
            for (int i = 0; i < 4; i++) {
                printf("{");
                for (int j = 0; j < 16; j++) {
                    printf(" %c,", cypherMap[i][j]);
                }
                printf("},\n");
            }
            printf("}\n");
            printf("cypherSortMap={");
            for (int i = 0; i < 4; i++) {
                printf("{");
                for (int j = 0; j < 16; j++) {
                    printf(" %c,", cypherSortMap[i][j]);
                }
                printf("},\n");
            }
            printf("}\n");

            printf("cypherPosMap:{");
            for (int i = 0; i < 4; i++) {
                printf("{");
                for (int j = 0; j < 16; j++) {
                    printf(" %d,", cypherPosMap[i][j]);
                }
                printf("},\n");
            }
        }

        static const char cypherMap[4][16]
            = { { '6', 'F', 'X', 'D', 'e', 'N', 'R', 'g', 'y', 's', '4', 'q', '=', '9', 'U', 'Y' },
                { '1', 'o', 'i', 'B', 'C', 'a', 'w', '3', 'j', 'v', '_', 'A', 'z', 'M', 'J', 'm' },
                { 'O', 'k', 'M', 't', 'd', '7', 'x', 'T', 'u', 'f', 'n', 'I', 'K', 'P', 'h', 'H' },
                { '5', 'L', '8', 'V', '0', 'E', 'b', 'S', 'l', 'G', 'Q', 'Z', 'r', 'c', '2', 'p' } };
        static const char cypherSortMap[4][16]
            = { { '4', '6', '9', '=', 'D', 'F', 'N', 'R', 'U', 'X', 'Y', 'e', 'g', 'q', 's', 'y' },
                { '1', '3', 'A', 'B', 'C', 'J', 'M', '_', 'a', 'i', 'j', 'm', 'o', 'v', 'w', 'z' },
                { '7', 'H', 'I', 'K', 'M', 'O', 'P', 'T', 'd', 'f', 'h', 'k', 'n', 't', 'u', 'x' },
                { '0', '2', '5', '8', 'E', 'G', 'L', 'Q', 'S', 'V', 'Z', 'b', 'c', 'l', 'p', 'r' } };
        static const int cypherPosMap[4][16] = { { 10, 0, 13, 12, 3, 1, 5, 6, 14, 2, 15, 4, 7, 11, 9, 8 },
                                                 { 0, 7, 11, 3, 4, 14, 13, 10, 5, 2, 8, 15, 1, 9, 6, 12 },
                                                 { 5, 15, 11, 12, 2, 0, 13, 7, 4, 9, 14, 1, 10, 3, 8, 6 },
                                                 { 4, 14, 0, 2, 5, 9, 1, 10, 7, 3, 11, 6, 13, 8, 15, 12 } };
        template <typename T>
        int bSearch(T * array, int size, T key)
        {
            int l = 0, h = size - 1;
            while (l <= h) {
                int mid = l + ((h - l) >> 1);
                if (key <= array[mid])
                    h = mid - 1;
                else
                    l = mid + 1;
            }
            if (!(l >= 0 && l < size && array[l] == key) || array[l] != key) {
                l = -1;
            }
            return l;
        }

        /**
        * 将给定字符串进行加密,字符串不超过8字节
        * @param input
        * @param size: input的大小
        * @param result:用于存放结果
        */
        void cookiesEncode(const uchar_t * input, const int32_t size, INOUT CypherResult128 & result)
        {
            if (size > 8) {
                throw MtException("cookiesEncode size > 8");
            }
            uchar_t * output = result.bytes;
            for (int i = 0, j = 0; i < size; i++) {
                output[j] = cypherMap[j & 0x03][input[i] & 0x0F];
                j++;
                output[j] = cypherMap[j & 0x03][(input[i] >> 4)];
                j++;
            }
            return;
        }

        /**
        * 将给定加密字符串进行解密
        * @param input
        * @param output : 输出结果
        * @param size: 存放结果大小
        */
        bool cookiesDecode(const CypherResult128 & input, INOUT uchar_t * output, INOUT int32_t & size)
        {
            if (size < 8) {
                return false;
            }
            static uchar_t mask[2] = { 0x0F, 0xF0 };
            const uchar_t * bytes = input.bytes;
            memset(output, 0, 8);
            for (int i = 0; i < 16; i++) {
                int j = i >> 1;
                int idx = bSearch<const char>(cypherSortMap[i & 0x03], 16, bytes[i]);
                if (idx < 0 || idx > 15)
                    return false;
                output[j] |= (cypherPosMap[i & 0x03][idx] << ((i & 0x01) ? 4 : 0)) & mask[i & 0x01];
            }
            size = 8;
            return true;
        }

        /**
        * 生成一个明文cookies串
        * @param result:用于存放结果
        * @param size:进入时指定result缓存的大小,返回时指出结果的长度,以字节为单位
        */
        void makeCookiesPublic(INOUT char_t * result, INOUT int32_t & size)
        {
            static std::random_device rd;
            static std::mt19937 randomGenerator(rd());
            union {
                int value;
                uchar_t bytes[4];
            } key1, key2;
            key1.value = time::getCurrentTimeSinceMtty();
            key2.value = randomGenerator();
            uchar_t midResult[8];
            uchar_t * p = midResult;
            for (int i = 0; i < 4; i++) {
                p[i] = key1.bytes[i];
            }
            p += 4;
            for (int i = 0; i < 4; i++) {
                p[i] = key2.bytes[i];
            }
            toHex(midResult, 8, result);
            size = 16;
        }

        /**
         * 生成一个加密的cookies串
         * @param result:用于存放结果
         * @param size:进入时指定result缓存的大小,返回时指出结果的长度,以字节为单位
         */
        void makeCookies(INOUT CypherResult128 & result)
        {
            static std::random_device rd;
            static std::mt19937 randomGenerator(rd());
            union {
                int value;
                uchar_t bytes[4];
            } key1, key2;
            key1.value = time::getCurrentTimeSinceMtty();
            key2.value = randomGenerator();
#ifdef VERBOSE_DEBUG
            printf("create cookies time:%p random:%p\n", key1.value, key2.value);
#endif
            uchar_t midResult[8];
            uchar_t * p = (uchar_t *)midResult;
            for (int i = 0; i < 4; i++) {
                p[i] = key1.bytes[i];
            }
            p += 4;
            for (int i = 0; i < 4; i++) {
                p[i] = key2.bytes[i];
            }
            cookiesEncode(midResult, 8, result);
            result.char_bytes[16] = '\0';
        }
    }
}
}
