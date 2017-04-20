#include "dsp_handler_manager.h"

namespace protocol {
namespace dsp {

    thread_local DSPHandlerManager dspHandlerManager;

    DSPHandlerManager::DSPHandlerManager()
    {
        handlers = { { 1234L, new DefaultDSPHandler(1234L, "http://www.google.com", 100) } };
    }

    DSPHandlerInterfacePtr DSPHandlerManager::getHandler(int64_t dspId)
    {
        auto iter = handlers.find(dspId);
        if (iter != handlers.end()) {
            return iter->second;
        }
        throw DSPHandlerException(std::stoi(dspId) + " handler not Found");
    }
}
}
