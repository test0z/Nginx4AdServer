#include "dsp_handler_manager.h"
#include <string>

namespace protocol {
namespace dsp {

    thread_local DSPHandlerManager dspHandlerManager;

    DSPHandlerManager::DSPHandlerManager()
        : executor("DSPExecutor")
    {
        addHandler(360L, std::make_shared<DefaultDSPHandler>(360L, "http://127.0.0.1:20007/mttybid",
                                                             "http://127.0.0.1:20007/m", 100));
        executor.start();
    }

    DSPHandlerInterfacePtr DSPHandlerManager::getHandler(int64_t dspId)
    {
        auto iter = handlers.find(dspId);
        if (iter != handlers.end()) {
            return iter->second;
        }
        throw DSPHandlerException(std::to_string(dspId) + " handler not Found");
    }
}
}
