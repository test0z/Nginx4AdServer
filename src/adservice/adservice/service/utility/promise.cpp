#include "promise.h"

namespace adservice {
namespace utility {
    namespace future {

        void Promise::done()
        {
            isDone = true;
            std::vector<PromiseListener> listenerQueue;
            std::unique_lock<std::mutex> lock(statusMutex);
            if (listeners.size() > 0) {
                listenerQueue.swap(listeners);
            }
            lock.unlock();
            for (auto & listener : listenerQueue) {
                try {
                    listener();
                } catch (...) {
                }
            }
        }

        void Promise::sync()
        {
            while (true) {
                std::unique_lock<std::mutex> lock(conditionMutex);
                bool ok = doneCondition.wait_for(lock, std::chrono::milliseconds(1), [this] {
                    bool allChildFinished = true;
                    for (PromisePtr & child : this->childPromises) {
                        if (!child->isDone) {
                            allChildFinished = false;
                            break;
                        }
                    }
                    return allChildFinished;
                });
                if (ok) {
                    break;
                }
            };
            done();
        }
    }
}
}
