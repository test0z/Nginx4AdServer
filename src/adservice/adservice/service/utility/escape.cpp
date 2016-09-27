//
// Created by guoze.lin on 16/2/23.
//

#include "escape.h"

namespace adservice {
    namespace utility {
        namespace escape {

            void numberEncode(int number, uint8_t *&buf) {
//                DebugMessage("numberEncode,",number);
                int i = 0;
                while (number > 0) {
                    buf[i] = (number & 0x7F) | 0x80;
                    number >>= 7;
                    i++;
                }
                buf[i - 1] &= 0x7F;
                buf = buf + i;
            }

            int numberDecode(const uint8_t *&buf) {
                int result = 0;
                int j = 0, i = 0;
                while (buf[i] > 0x7F)i++;
                j = i;
                while (i > 0) {
                    result += buf[i] & 0x7F;
                    result <<= 7;
                    i--;
                }
                result += buf[0] & 0x7F;
                buf += j + 1;
//                DebugMessage("numberDecode,",result);
                return result;
            }

            std::string encode4ali(const std::string& input){
                return encode4ali(input.c_str(),input.length());
            }

            std::string encode4ali(const char* buf,int len) {
                int zeroIndex[1024];
                int zeroCnt = 0;
                for (int i = 0; i < len; i++) {
                    if (buf[i] == '\0') {
                        zeroIndex[zeroCnt++] = i + 1;
                    }
                }
                char stub[1024];
                if (zeroCnt == 0) {
                    stub[0] = '0';
                    memcpy(stub + 1, buf, len);
                    return std::string(stub, stub + len + 1);
                }
                stub[0] = '1';
                uint8_t *stubbuf = (uint8_t *) (stub + 1);
                numberEncode(zeroCnt, stubbuf);
                for (int i = 0; i < zeroCnt; i++) {
                    numberEncode(zeroIndex[i], stubbuf);
                }
                for (int i = 0; i < len; i++) {
                    if (buf[i] != '\0') {
                        *stubbuf = buf[i];
                        stubbuf++;
                    }
                }
                *stubbuf = 0;
                return std::string(stub, (char *) stubbuf);
            }


            std::string decode4ali(const std::string& input){
                return decode4ali(input.c_str(),input.length());
            }

            std::string decode4ali(const char* inputBuf,int len) {
                if (inputBuf[0] == '0') {
                    return std::string(inputBuf + 1, inputBuf + len);
                }
                const uint8_t *p = (const uint8_t *) (inputBuf + 1);
                int zeroIdx[1024];
                int zeroCnt = numberDecode(p);
                char result[1024];
                for (int i = 0; i < zeroCnt; i++) {
                    zeroIdx[i] = numberDecode(p) - 1;
                }
                int i, j, k;
                for (i = 0, j = 0, k = 0; j < zeroCnt && p[i] != 0;) {
                    while (k < zeroIdx[j]) {
                        result[k++] = p[i++];
                    }
                    result[k++] = 0;
                    j++;
                }
                while(j<zeroCnt){
                    result[k++] = 0;
                    j++;
                }
                while (p[i] != 0) {
                    result[k++] = p[i++];
                }
                result[k] = 0;
                return std::string(result, result + k);
            }

        }
    }
}
