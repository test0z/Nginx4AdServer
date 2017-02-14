
#include "360max_price.h"
#include "logging.h"

int64_t max360_price_decode(const std::string & input)
{
    try {
        return std::stoi(input);
    } catch (std::exception & e) {
        LOG_ERROR << "max360_price_decode error,e:" << e.what() << ",input:" << input;
    }
    return 0;
}
