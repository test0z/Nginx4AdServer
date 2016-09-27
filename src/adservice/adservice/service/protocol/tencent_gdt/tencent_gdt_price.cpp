//
// Created by guoze.lin on 16/5/11.
//

#include "tencent_gdt_price.h"
#include <exception>
#include "utility/utility.h"
#include "common/functions.h"
#include "logging.h"

using namespace adservice::utility::cypher;
using namespace adservice::utility::url;
using namespace Logging;

//static const char* TOKEN = "NDAzMzY3LDE0MjI4";
static const char* TOKEN = "MTI3NzU4MywxNDUz";

class GdtPriceDecoder{
public:
    GdtPriceDecoder(const char* token){
        keySize = strlen(TOKEN);
        memcpy(key,TOKEN,keySize);
        key[keySize] = 0;
    }
    int getPrice(const std::string& input){
        std::string result;
        std::string base64decode;
        std::string paddingString = input+padding[input.length()%4];
        urlsafe_base64decode(input,base64decode);
        aes_ecbdecode_nopadding(key,base64decode,result);
        return std::stoi(result);
    }
private:
    uchar_t key[32];
    int keySize;
private:
    static std::string padding[4];
};

std::string GdtPriceDecoder::padding[4]={"","===","==","="};

static GdtPriceDecoder decoder(TOKEN);

int gdt_price_decode(const std::string& input){
    int price = 0;
    try {
        char buffer[2048];
        std::string output;
        urlDecode_f(input,output,buffer);//Maybe twice?
        price = decoder.getPrice(output);
        //DebugMessageWithTime("gdt price input:",input,",price:",price);
    }catch(std::exception& e){
        LOG_ERROR<<"gdt_price_decode failed,input:"<<input;
    }
    return price;
}
