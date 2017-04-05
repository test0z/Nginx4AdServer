//
// Created by guoze.lin on 16/1/20.
//

#ifndef ADCORE_LOG_H
#define ADCORE_LOG_H

#include "protocol/log/log.h"
#include <cstdlib>
#include <exception>
#include <memory>
#include <sys/types.h>

#ifdef NO_COW_STRING //如果定义了禁止string的COW,那么使用vstring或者请在Makefile中定义_GLIBCXX_USE_CXX11_ABI=1
#include <ext/vstring.h>
#include <ext/vstring_fwd.h>

namespace adservice {
namespace types {

    // c++11 small string optimization
    // https://gcc.gnu.org/ml/gcc-patches/2014-11/msg01785.html
    typedef __gnu_cxx::__sso_string string;
}
}
#else

namespace adservice {
namespace types {
    typedef std::string string;
}
}

#endif

#ifndef char_t
typedef char char_t;
#endif

#ifndef uchar_t
typedef u_char uchar_t;
#endif

#ifndef uint8_t
typedef u_int8_t uint8_t;
#endif

#ifndef uint16_t
typedef u_int16_t uint16_t;
#endif

#ifndef uint32_t
typedef u_int32_t uint32_t;
#endif

#ifndef uint64_t
typedef u_int64_t uint64_t;
#endif

#define IN
#define OUT
#define INOUT

class MtException : public std::exception {
public:
    MtException(const std::string & message) noexcept
        : msg(message)
    {
    }
    virtual ~MtException() noexcept
    {
    }
    virtual const char * what() const noexcept
    {
        return msg.data();
    }

protected:
    std::string msg;
};

#endif
