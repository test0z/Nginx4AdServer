#include "nex_price.h"
#include "logging.h"
#include "utility/utility.h"

using namespace adservice::utility::cypher;

int nex_price_decode(const std::string & input)
{
    static const char * key = "OGEyY2EwZGI1Y2Nm"; //"OGEyNTQ0Zjc1OTI1";
    static const char * iv = "MDE1Y2NmMTliZmFi";  //"MDE1OTI2MGExZjEw";
    try {
        std::string decode;
        base64decode(input, decode);
        std::string aesResult;
        aes_cbcdecode((const uchar_t *)key, (const uchar_t *)iv, decode, aesResult);
        return std::stoi(aesResult);
    } catch (std::exception & e) {
        LOG_ERROR << "nex_price_decode exception,e:" << e.what() << ",input:" << input;
    }
    return 0;
}
