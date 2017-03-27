#ifndef COMPRESS_H
#define COMPRESS_H

#include <string>

namespace adservice {
namespace utility {
    namespace gzip {

        std::string compress(std::stringstream & origin);

        std::string compress(const std::string & data);

        std::string decompress(std::stringstream & compressed);

        std::string decompress(const std::string & data);
    }
}
}

#endif // COMPRESS_H
