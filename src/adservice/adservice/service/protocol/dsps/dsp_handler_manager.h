#ifndef DSP_HANDLER_MANAGER_H__
#define DSP_HANDLER_MANAGER_H__

#include "core/core_executor.h"
#include "dsp_handlers/default_dsp_handler.h"
#include <booster/backtrace.h>
#include <unordered_map>

namespace protocol {
namespace dsp {

    class DSPHandlerException : public booster::exception {
    public:
        DSPHandlerException(const std::string & what)
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

    typedef std::shared_ptr<DSPHandlerInterface> DSPHandlerInterfacePtr;

    class DSPHandlerManager {
    public:
        DSPHandlerManager();

        ~DSPHandlerManager()
        {
            executor.stop();
        }

        DSPHandlerInterfacePtr getHandler(int64_t dspId);

        adservice::server::Executor & getExecutor()
        {
            return executor;
        }

    private:
        void addHandler(int64_t id, DSPHandlerInterfacePtr handler)
        {
            handlers.insert({ id, handler });
        }

    private:
        adservice::server::Executor executor;
        std::unordered_map<int64_t, DSPHandlerInterfacePtr> handlers;
    };
}
}

#endif
