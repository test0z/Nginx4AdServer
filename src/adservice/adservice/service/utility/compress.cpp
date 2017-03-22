#include "compress.h"

#include "logging.h"
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <sstream>

namespace adservice {
namespace utility {
    namespace gzip {

        std::string compress(const std::string & data)
        {
            std::stringstream origin(data);
            return compress(origin);
        }

        std::string compress(std::stringstream & origin)
        {
            try {
                std::stringstream compressed;
                boost::iostreams::filtering_streambuf<boost::iostreams::input> out;
                out.push(boost::iostreams::gzip_compressor(
                    boost::iostreams::gzip_params(boost::iostreams::gzip::best_compression)));
                out.push(origin);
                boost::iostreams::copy(out, compressed);

                return compressed.str();
            } catch (std::exception & e) {
                LOG_WARN << "failed to compress input:" << origin.str();
                return origin.str();
            }
        }

        std::string decompress(const std::string & data)
        {
            std::stringstream compressed(data);
            return decompress(compressed);
        }

        std::string decompress(std::stringstream & compressed)
        {
            try {
                std::stringstream decompressed;
                boost::iostreams::filtering_streambuf<boost::iostreams::input> out;
                out.push(boost::iostreams::gzip_decompressor());
                out.push(compressed);
                boost::iostreams::copy(out, decompressed);
                return decompressed.str();
            } catch (std::exception & e) {
                LOG_WARN << "failed to decompress input:" << compressed.str();
                return compressed.str();
            }
        }
    }
}
}
