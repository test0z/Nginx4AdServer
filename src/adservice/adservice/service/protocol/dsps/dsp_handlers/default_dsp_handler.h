#ifndef DEFAULT_DSP_HANDLER_H__
#define DEFAULT_DSP_HANDLER_H__

#include "core/adselectv2/ad_select_interface.h"
#include "protocol/dsps/mtty_bidding.pb.h"
#include <mtty/types.h>
#include <queue>
#include <string>

namespace protocol {
namespace dsp {

    using namespace adservice;

    class DSPBidResult {
    public:
        void reset()
        {
            dspId = 0;
            resultOk = false;
            bidPrice = 0.0;
            banner = MT::common::Banner();
        }
        int64_t dspId;
        bool resultOk{ false };
        double bidPrice{ 0.0 };
        MT::common::Banner banner;
    };

    typedef std::function<void(void)> DSPPromiseListener;

    class DSPPromise;

    typedef std::shared_ptr<DSPPromise> DSPPromisePtr;

    class DSPPromise {
    public:
        void addListener(const DSPPromiseListener & listener)
        {
            if (this->isDone) {
                listener();
            } else {
                std::lock_guard<std::mutex> lockGuard(statusMutex);
                listeners.push(listener);
            }
        }

        void addListener(const DSPPromiseListener && listener)
        {
            if (this->isDone) {
                listener();
            } else {
                std::lock_guard<std::mutex> lockGuard(statusMutex);
                listeners.push(listener);
            }
        }

        void after(const DSPPromisePtr & promise)
        {
            if (promise) {
                promise->addListener([this]() { this->wakeup(); });
                {
                    std::lock_guard<std::mutex> lockGuard(statusMutex);
                    childPromises.push_back(promise);
                }
            }
        }

        void done()
        {
            isDone = true;
            std::queue<DSPPromiseListener> listenerQueue;
            {
                std::lock_guard<std::mutex> lock(statusMutex);
                listenerQueue.swap(listeners);
            }
            while (!listenerQueue.empty()) {
                lock.lock();
                auto & listener = listenerQueue.front();
                listenerQueue.pop();
                lock.unlock();
                try {
                    listener();
                } catch (...) {
                }
            }
        }

        bool isFinished()
        {
            return this->isDone;
        }

        void wakeup()
        {
            doneCondition.notify_all();
        }

        void sync()
        {
            while (true) {
                bool allChildFinished = true;
                for (DSPPromisePtr & child : childPromises) {
                    if (!child->isDone) {
                        allChildFinished = false;
                        break;
                    }
                }
                if (!allChildFinished) {
                    std::unique_lock<std::mutex> lock(conditionMutex);
                    doneCondition.wait_for(lock, 1); //有可能出现先notify但是后wait的情况，需要加屏障
                } else {
                    break;
                }
            };
            done();
        }

    private:
        volatile bool isDone{ false };
        std::mutex conditionMutex;
        std::mutex statusMutex;
        std::condition_variable doneCondition;
        std::vector<DSPPromisePtr> childPromises;
        std::queue<DSPPromiseListener> listeners;
    };

    class DSPHandlerInterface {
    public:
        DSPHandlerInterface(int64_t dspId, const std::string & targetUrl, int64_t timeout)
            : dspId_(dspId)
            , dspUrl_(targetUrl)
            , timeoutMs_(timeout)
        {
        }

        /**
         * 给DSP发送异步 BID 请求
         * @brief sendBid
         * @param selectCondition
         */
        virtual DSPPromisePtr sendBid(adselectv2::AdSelectCondition & selectCondition,
                                      const MT::common::ADPlace & adplace);

        const DSPBidResult & getResult()
        {
            return dspResult;
        }

        const std::string & getDspUrl()
        {
            return dspUrl;
        }

        int64_t getDSPId()
        {
            return dspId;
        }

        int64_t getTimeout()
        {
            return timeoutMs;
        }

    protected:
        BidRequest conditionToBidRequest(adselectv2::AdSelectCondition & selectCondition,
                                         const MT::common::ADPlace & adplace);

        DSPBidResult bidResponseToDspResult(BidResponse & bidResponse);

    protected:
        std::string dspUrl_;
        int64_t dspId_;
        int64_t timeoutMs_;
        DSPBidResult dspResult_;
    };

    typedef DSPHandlerInterface DefaultDSPHandler;
}
}

#endif
