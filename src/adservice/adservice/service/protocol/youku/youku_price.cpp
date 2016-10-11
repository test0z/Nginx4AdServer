//
// Created by guoze.lin on 16/4/8.
//

#include "youku_price.h"
#include "common/functions.h"
#include "logging.h"
#include "utility/utility.h"
#include <exception>

using namespace adservice::utility::cypher;
using namespace adservice::utility::url;

static const char * TOKEN = "c28dc5ce331840668de6d1ac9be0b0a8";
// const char* TOKEN = "e48c06fe0da2403db2de26e2fcfe14d5";

class YoukuPriceDecoder {
public:
	YoukuPriceDecoder(const char * token)
	{
        keySize = 32;
		fromLittleEndiumHex(token, strlen(token), key, keySize, false);
    }
	int getPrice(const std::string & input)
	{
        std::string result;
        std::string base64decode;
		std::string paddingString = input + padding[input.length() % 4];
		urlsafe_base64decode(paddingString, base64decode);
		aes_ecbdecode(key, base64decode, result);
        return std::stoi(result);
    }

private:
    uchar_t key[32];
    int keySize;

private:
    static std::string padding[4];
};

std::string YoukuPriceDecoder::padding[4] = { "", "===", "==", "=" };

static YoukuPriceDecoder decoder(TOKEN);

int64_t youku_price_decode(const std::string & input)
{
    int price = 0;
    try {
        char buffer[2048];
        std::string output;
		urlDecode_f(input, output, buffer); // Maybe twice?
        price = decoder.getPrice(output);
	} catch (std::exception & e) {
		LOG_ERROR << "youku_price_decode failed,input:" << input;
    }
    return price;
}
