#ifndef PROMISE_H
#define PROMISE_H

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

namespace adservice {
namespace utility {

    namespace future {

        typedef std::function<void(void)> PromiseListener;

        class Promise;

        typedef std::shared_ptr<Promise> PromisePtr;

        class Promise {
        public:
            void addListener(const PromiseListener & listener)
            {
                if (this->isDone) {
                    listener();
                } else {
                    std::lock_guard<std::mutex> lockGuard(statusMutex);
                    listeners.push_back(listener);
                }
            }

            void addListener(const PromiseListener && listener)
            {
                if (this->isDone) {
                    listener();
                } else {
                    std::lock_guard<std::mutex> lockGuard(statusMutex);
                    listeners.push_back(listener);
                }
            }

            void after(const PromisePtr & promise)
            {
                if (promise) {
                    promise->addListener([this]() { this->wakeup(); });
                    {
                        std::lock_guard<std::mutex> lockGuard(statusMutex);
                        childPromises.push_back(promise);
                    }
                }
            }

            void done();

            bool isFinished()
            {
                return this->isDone;
            }

            void wakeup()
            {
                std::unique_lock<std::mutex> lock(conditionMutex);
                doneCondition.notify_all();
            }

            void sync();

        private:
            volatile bool isDone{ false };
            std::mutex conditionMutex;
            std::mutex statusMutex;
            std::condition_variable doneCondition;
            std::vector<PromisePtr> childPromises;
            std::vector<PromiseListener> listeners;
        };
    }
}
}

#endif // PROMISE_H
