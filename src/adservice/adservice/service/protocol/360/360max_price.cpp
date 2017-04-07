
#include "360max_price.h"
#include "logging.h"
#include <iostream>
#include <sstream>

#include <openssl/hmac.h>

#include <boost/algorithm/string.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>

#include <booster/backtrace.h>

typedef boost::archive::iterators::
    transform_width<boost::archive::iterators::binary_from_base64<std::string::const_iterator>, 8, 6>
        Base64DecodeIterator;

namespace {

class PriceDecodeException : public booster::exception {
public:
    PriceDecodeException(const std::string & what)
        : what_(what)
    {
    }

    const char * what() const noexcept override
    {
        return what_.c_str();
    }

private:
    std::string what_;
};

int toDigit(char ch, int index) throw(PriceDecodeException)
{
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    } else if (ch >= 'a' && ch <= 'f') {
        return ch + 10 - 'a';
    } else if (ch >= 'A' && ch <= 'F') {
        return ch + 10 - 'A';
    } else {
        throw PriceDecodeException("非法的16进制字符：" + std::string(1, ch) + "，位置：" + std::to_string(index));
    }
}

std::vector<uint8_t> decodeHex(const std::string & data) throw(PriceDecodeException)
{
    auto len = data.length();
    if ((len & 1) != 0) {
        throw PriceDecodeException("decodeHex：奇数个16进制位。");
    } else {
        std::vector<uint8_t> out(len >> 1);
        int i = 0;

        for (uint j = 0; j < len; ++i) {
            int f = toDigit(data[j], j) << 4;
            ++j;
            f |= toDigit(data[j], j);
            ++j;
            out[i] = (uint8_t)(f & 255);
        }

        return out;
    }
}

std::vector<uint8_t> hmac(const std::vector<uint8_t> & data, const std::vector<uint8_t> & key,
                          const std::string & algorithm) throw(PriceDecodeException)
{
    const EVP_MD * engine = NULL;
    if (algorithm == "HmacSHA512") {
        engine = EVP_sha512();
    } else if (algorithm == "HmacSHA256") {
        engine = EVP_sha256();
    } else if (algorithm == "HmacSHA1") {
        engine = EVP_sha1();
    } else if (algorithm == "HmacMD5") {
        engine = EVP_md5();
    } else if (algorithm == "HmacSHA224") {
        engine = EVP_sha224();
    } else if (algorithm == "HmacSHA384") {
        engine = EVP_sha384();
    } else if (algorithm == "HmacSHA") {
        engine = EVP_sha();
    } else {
        throw PriceDecodeException("不支持的Hmac算法：" + algorithm);
    }

    std::vector<uint8_t> output(EVP_MAX_MD_SIZE);

    HMAC_CTX ctx;
    HMAC_CTX_init(&ctx);
    HMAC_Init_ex(&ctx, key.data(), (int)key.size(), engine, nullptr);
    HMAC_Update(&ctx, data.data(), data.size());

    unsigned len = 0;
    HMAC_Final(&ctx, output.data(), &len);
    HMAC_CTX_cleanup(&ctx);

    return output;
}

std::vector<uint8_t> xorBytes(const std::vector<uint8_t> & bytes1, const std::vector<uint8_t> & bytes2, int length)
{
    std::vector<uint8_t> result(length);
    for (int i = 0; i < length; i++) {
        result[i] = (bytes1[i] ^ bytes2[i]);
    }
    return result;
}

double decodePrice(const std::string & dec_key, const std::string & i_key,
                   std::string e_enc) throw(PriceDecodeException)
{
    // base64解码
    std::vector<uint8_t> e_src;
    try {
        boost::replace_all(e_enc, "-", "+");
        boost::replace_all(e_enc, "_", "/");
        std::copy(Base64DecodeIterator(e_enc.begin()), Base64DecodeIterator(e_enc.end()), std::back_inserter(e_src));
    } catch (std::exception & e) {
        throw PriceDecodeException("Base64解码失败。原文：" + e_enc + "，错误原因：" + e.what());
    }

    std::vector<uint8_t> iv(e_src.begin(), e_src.begin() + 16);              // 初始化矢量数组
    std::vector<uint8_t> en_price(e_src.begin() + 16, e_src.begin() + 24);   // 加密的价格数组
    std::vector<uint8_t> integrity(e_src.begin() + 24, e_src.begin() + 28);  // 完整性签名的前4位
    std::vector<uint8_t> de_byte = hmac(iv, decodeHex(dec_key), "HmacSHA1"); // 加密得到加密数组
    std::vector<uint8_t> dec_price = xorBytes(en_price, de_byte, 8);         // 通过异或运算得到价格数组

    // 把价格数组按照大端字节序转换成价格
    int64_t price;
    std::vector<uint8_t> dec_price_bigendian;
    for (auto rit = dec_price.rbegin(); rit != dec_price.rend(); ++rit) {
        dec_price_bigendian.push_back(*rit);
    }
    memcpy(&price, dec_price_bigendian.data(), 8);

    // 将价格数组和时间戳数组，合成一个12位的数组
    std::vector<uint8_t> dec_priceAndTime(dec_price.begin(), dec_price.end());
    dec_priceAndTime.insert(dec_priceAndTime.end(), iv.begin(), iv.end());

    std::vector<uint8_t> o_ikey = hmac(dec_priceAndTime, decodeHex(i_key), "HmacSHA1"); // 加密得到原始完整性校验数组

    // 比较两个签名数组是否相同
    bool flag = true;
    for (int i = 0; i < 4; ++i) {
        if (integrity[i] != o_ikey[i]) {
            flag = false;
        }
    }

    if (!flag) {
        throw PriceDecodeException("签名验证失败"); // 返回签名非法的错误
    }
    return price;
}
}

static const std::string DEC_KEY = "890ec02651413eb78f4569c76821397d7323ca10538d46b17ddbb08a5ac5c343";
static const std::string IKEY = "f564fb940597adf219c59dbff5025d206a32a9ab3d94f68def0a793ccc397e1a";

int64_t max360_price_decode(const std::string & input)
{
    try {
        double result = decodePrice(DEC_KEY, IKEY, input);
        return (int64_t)result;
    } catch (PriceDecodeException & pe) {
        LOG_ERROR << "max360_price_decode price error,input:" << input << ",e:" << pe.trace();
    } catch (std::exception & e) {
        LOG_ERROR << "max360_price_decode price error,input:" << input << ",e:" << e.what();
    }
    return 0;
}
