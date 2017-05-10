#include "http.h"
#include "logging.h"
#include "url.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace adservice {
namespace utility {

    void HttpResponse::set_body(const std::string & bodyMessage)
    {
        if (bodyMessage.length() == 0)
            return;
        body = bodyMessage;
    }

    std::string HttpResponse::get_body() const
    {
        return body;
    }

    std::string HttpResponse::get_debug_message()
    {
        if (bodyStream.tellp() > 0)
            return bodyStream.str();
        else
            return "";
    }

    std::shared_ptr<HttpClientProxy> HttpClientProxy::instance_ = nullptr;

    HttpResponseCallback HttpClientProxy::defaultResponseCallback_ = [](int, int, const std::string &) {};

    namespace {

        class ConnectionInfo {
        public:
            CURL * curl{ nullptr };
            std::string url;
            std::string postData;
            std::stringstream dataBuffer;
            std::shared_ptr<HttpClientCallbackPack> callbackInfo;

            ConnectionInfo()
            {
                callbackInfo = std::make_shared<HttpClientCallbackPack>();
            }
        };

        struct CurlSocketInfo {
            // curl socket
            curl_socket_t sockfd;
            // curl socket对应的句柄
            CURL * easy;
            //当前的需要监听的event
            int32_t event;
            //当前会话的超时控制
            ConnectionInfo * connInfo{ nullptr };
            //事件循环中注册的事件句柄
            struct ev_io eventHandle;
            //是否已经在时间循环中注册事件
            bool eventRegistered{ false };
        };

        /**
         * todo:优化 放到executor worker中执行
         * 清理已经完成的easy handle
         * @brief clean_finished_handles
         */
        void clean_finished_handles()
        {
            CURLMsg * msg;
            int handleRemains;
            CURL * easy;
            CURLM * curlm = HttpClientProxy::instance_->getCurlM();
            ConnectionInfo * connectionInfo{ nullptr };
            while ((msg = curl_multi_info_read(curlm, &handleRemains))) {
                if (msg->msg == CURLMSG_DONE) { //已完成清理easy handle
                    easy = msg->easy_handle;
                    CURLcode res = msg->data.result;
                    int err = 0;
                    if (res != CURLE_OK) { // transfer failed
                        err = res;
                    } // else transfer ok
                    curl_easy_getinfo(easy, CURLINFO_PRIVATE, &connectionInfo);
                    int64_t responseCode;
                    curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &responseCode);
                    if (connectionInfo != nullptr) {
                        HttpClientProxy::instance_->getExecutor()->run([connectionInfo, err, responseCode]() {
                            auto callbackInfo = connectionInfo->callbackInfo;
                            bool called = callbackInfo->callbackCalled.exchange(true);
                            if (!called) {
                                if (callbackInfo->transferFinishedCallback) { //调用连接完成回调
                                    callbackInfo->transferFinishedCallback(err, responseCode,
                                                                           connectionInfo->dataBuffer.str());
                                }
                                callbackInfo->promisePtr->done();
                            }
                            delete connectionInfo;
                        });
                    }
                    curl_multi_remove_handle(curlm, easy);
                    curl_easy_cleanup(easy);
                }
            }
        }

        /**
         * 对超时未处理的curl_easy句柄进行激活。
         * 对于初次提交的请求,将延迟1ms进行激活；如果要及时激活,产生请求后应该马上调用curl_multi_socket_action
         * @brief curlm_wakeup_timer_callback
         * @param loop
         */
        void curlm_wakeup_timer_callback(EV_P_ struct ev_timer *, int)
        {
            CURLMcode rc;
            CURLM * curlm = HttpClientProxy::instance_->getCurlM();
            int remains = 0;
            //检查挂载在curlm上的curl_easy句柄的状态,并引发curlm上的SOCKET CALLBACK的调用
            rc = curl_multi_socket_action(curlm, CURL_SOCKET_TIMEOUT, 0, &remains);
            if (rc != CURLM_OK && rc != CURLM_BAD_SOCKET) {
                LOG_WARN << "curlm not ok,function:curlm_wakeup_timer_callback,rc:" << rc;
            }
            // 检查是否已经完成
            clean_finished_handles();
        }

        void socket_event_listener(EV_P_ struct ev_io * w, int revents)
        {
            CurlSocketInfo * socketInfo = (CurlSocketInfo *)w->data;
            int action = (revents & EV_READ ? CURL_POLL_IN : 0) | (revents & EV_WRITE ? CURL_POLL_OUT : 0);
            int remains = 0;
            // epoll通知发生了事件,curl底层按照状态处理对应的逻辑(比如调用write callback)
            CURLMcode rc = curl_multi_socket_action(HttpClientProxy::instance_->getCurlM(), socketInfo->sockfd, action,
                                                    &remains);
            if (rc != CURLM_OK && rc != CURLM_BAD_SOCKET) {
                LOG_WARN << "curlm not ok,function:socket_event_listener,rc:" << rc;
            }
            // 检查是否已经完成
            clean_finished_handles();
        }

        /**
         * 对状态发生改变的curl_socket进行维护,
         * 对于新socket,status = 0, 将它的读写状态变更事件代理给epoll
         * 对于老socket,status 为可读可写，如果和上次状态不一样意味着epoll监听的事件需要更改
         * 对于老socket，status 为移除,删除在epoll的监听
         * @brief socket_status_change_callback
         * @param e
         * @param s
         * @param status
         * @param socketData
         * @return
         */
        int socket_status_change_callback(CURL * easyHandle, curl_socket_t sockfd, int status, void *,
                                          void * socketData)
        {
            CurlSocketInfo * socketInfo = (CurlSocketInfo *)socketData;
            if (status == CURL_POLL_REMOVE) { // socket 移除
                // LOG_TRACE << "socket removed";
                if (socketInfo) {
                    if (socketInfo->eventRegistered) {
                        ev_io_stop(HttpClientProxy::instance_->loop(), &socketInfo->eventHandle);
                    }
                    delete socketInfo;
                }
            } else {
                if (socketInfo == nullptr) { //新建socket
                    // LOG_TRACE << "socket status : new socket";
                    socketInfo = new CurlSocketInfo;
                    socketInfo->easy = easyHandle;
                    socketInfo->sockfd = sockfd;
                    socketInfo->event = (status & CURL_POLL_IN ? EV_READ : 0) | (status & CURL_POLL_OUT ? EV_WRITE : 0);
                    curl_easy_getinfo(easyHandle, CURLINFO_PRIVATE, &socketInfo->connInfo);
                    if (socketInfo->eventRegistered) {
                        ev_io_stop(HttpClientProxy::instance_->loop(), &socketInfo->eventHandle);
                    }
                    ev_io_init(&socketInfo->eventHandle, socket_event_listener, socketInfo->sockfd, socketInfo->event);
                    socketInfo->eventHandle.data = socketInfo;
                    socketInfo->eventRegistered = true;
                    ev_io_start(HttpClientProxy::instance_->loop(), &socketInfo->eventHandle);
                    curl_multi_assign(HttpClientProxy::instance_->getCurlM(), sockfd, socketInfo);
                } else { // socket 可读或可写状态
                    // LOG_TRACE << "socket readable or writable";
                    int32_t newEvent = (status & CURL_POLL_IN ? EV_READ : 0) | (status & CURL_POLL_OUT ? EV_WRITE : 0);
                    if (newEvent != socketInfo->event) {
                        socketInfo->event = newEvent;
                        if (socketInfo->eventRegistered) {
                            ev_io_stop(HttpClientProxy::instance_->loop(), &socketInfo->eventHandle);
                        }
                        ev_io_init(&socketInfo->eventHandle, socket_event_listener, socketInfo->sockfd,
                                   socketInfo->event);
                        socketInfo->eventHandle.data = socketInfo;
                        socketInfo->eventRegistered = true;
                        ev_io_start(HttpClientProxy::instance_->loop(), &socketInfo->eventHandle);
                    }
                }
            }
            return 0;
        }

        int multi_timer_cb(CURLM *, long timeoutMs, void *)
        {
            // -1 删除timer,curl_multi_socket_action可调用也可不调用
            // 0 过期,需要调用curl_multi_socket_action
            // 大于0 设置timer
            // LOG_TRACE << "multi_timer_cb timeout ms:" << timeoutMs << ",threadId:" << std::this_thread::get_id();
            ev_timer_stop(HttpClientProxy::instance_->loop(), HttpClientProxy::instance_->wakeupTimer());
            if (timeoutMs > 0) {
                ev_timer_init(HttpClientProxy::instance_->wakeupTimer(), curlm_wakeup_timer_callback,
                              timeoutMs / 1000.0, 0.);
                ev_timer_start(HttpClientProxy::instance_->loop(), HttpClientProxy::instance_->wakeupTimer());
            } else
                curlm_wakeup_timer_callback(HttpClientProxy::instance_->loop(),
                                            HttpClientProxy::instance_->wakeupTimer(), 0);
            return 0;
        }

        size_t data_write_callback(char * ptr, size_t size, size_t nmemb, void * userdata)
        {
            int actualSize = size * nmemb;
            if (actualSize > 0) {
                ConnectionInfo * connInfo = (ConnectionInfo *)userdata;
                connInfo->dataBuffer << std::string(ptr, ptr + actualSize);
            }
            return actualSize;
        }

        CURL * newCurl(ConnectionInfo * connInfo, bool verbose = false)
        {
            CURL * curl = curl_easy_init();
            if (curl == nullptr) {
                throw HttpClientException("failed to create new curl object");
            }
            curl_easy_setopt(curl, CURLOPT_URL, (char *)connInfo->url.data());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, data_write_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, connInfo);
            curl_easy_setopt(curl, CURLOPT_PRIVATE, connInfo);
            curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
            curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L);
            curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
            if (verbose) {
                curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
            }
            return curl;
        }

        void default_event_listener(EV_P_ struct ev_io * w, int revents)
        {
        }

        void task_timer_listener(EV_P_ struct ev_timer *, int)
        {
            if (HttpClientProxy::instance_.use_count() <= 0) {
                return;
            }
            std::mutex & m = HttpClientProxy::instance_->taskMutex();
            std::vector<CURL *> & curls = HttpClientProxy::instance_->pendingCurls();
            std::vector<CURL *> deliveredCurls;
            CURLM * curlm = HttpClientProxy::instance_->getCurlM();
            TimeoutQueue & timeoutQueue = HttpClientProxy::instance_->getTimeoutQueue();
            int64_t currentTime = time::getCurrentTimeStampMs();
            HttpClientCallbackPackPtr earliestCallback = nullptr;
            {
                std::lock_guard<std::mutex> lock(m);
                deliveredCurls.swap(curls);
                if (timeoutQueue.size() > 0) {
                    earliestCallback = timeoutQueue.top();
                }
            }
            if (!deliveredCurls.empty()) {
                for (CURL * curl : deliveredCurls) {
                    CURLMcode rc = curl_multi_add_handle(curlm, curl);
                    if (rc != CURLM_OK) {
                        LOG_ERROR << "curl_multi_add_handle failed in HttpClientProxy::getAsync,retCode:" << rc;
                    }
                }
            }
            //检查超时列表的头部
            std::vector<HttpClientCallbackPackPtr> toCalls;
            if (earliestCallback != nullptr && earliestCallback->deadline < currentTime) {
                std::lock_guard<std::mutex> lock(m);
                while (timeoutQueue.size() > 0) {
                    const HttpClientCallbackPackPtr & callbackInfo = timeoutQueue.top();
                    if (callbackInfo->deadline > currentTime) {
                        break;
                    } else {
                        toCalls.push_back(callbackInfo);
                        timeoutQueue.pop();
                    }
                }
            }
            for (auto & callbackInfo : toCalls) {
                bool called = callbackInfo->callbackCalled.exchange(true);
                if (!called) {
                    HttpClientProxy::instance_->getExecutor()->run([callbackInfo]() {
                        if (callbackInfo->transferFinishedCallback) {
                            callbackInfo->transferFinishedCallback(CURLE_OPERATION_TIMEDOUT, 0, "");
                        }
                        callbackInfo->promisePtr->done();
                    });
                }
            }
        }

        int timeout_checker(void * clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
        {
            ConnectionInfo * connInfo = (ConnectionInfo *)clientp;
            if (connInfo == nullptr) {
                return -1;
            }
            int64_t currentTime = time::getCurrentTimeStampMs();
            // todo 超时回调放到executor worker中执行
            auto callbackInfo = connInfo->callbackInfo;
            if (callbackInfo->deadline > 0 && currentTime > callbackInfo->deadline) {
                // LOG_DEBUG << std::this_thread::get_id() << " timeout checker,time is up";
                bool called = callbackInfo->callbackCalled.exchange(true);
                if (!called) {
                    HttpClientProxy::instance_->getExecutor()->run([callbackInfo]() {
                        //调用连接完成回调
                        if (callbackInfo->transferFinishedCallback) {
                            callbackInfo->transferFinishedCallback(CURLE_OPERATION_TIMEDOUT, 0, "");
                        }
                        callbackInfo->promisePtr->done();
                    });
                }
                //因为libcurl 内部progress
                // callback总是限于CURM_MSG::DONE的设置，所以这里无需删除connInfo,等CURM_MSG::DONE再删
                return -1;
            }
            return 0;
        }
    }
    // todo:目前没有额外IO线程,所有的IO都集中在libev的loop thread中,下一步的优化需要加入额外的IOthread,是的libev loop
    // 足够轻
    HttpClientProxy::HttpClientProxy()
    {
        executor_ = std::make_shared<adservice::server::Executor>("HttpClientProxyExecutor", true, 3, 5000);
        executor_->start();
        loop_ = ev_loop_new(0);
        multiCurl_ = curl_multi_init();
        memset(&wakeupTimer_, 0, sizeof(wakeupTimer_));
        ev_timer_init(&wakeupTimer_, curlm_wakeup_timer_callback, 0.0, 0.0);
        ev_timer_init(&taskTimer_, task_timer_listener, 0.0, 1 / 1000.0);
        ev_timer_start(loop_, &taskTimer_);
        curl_multi_setopt(multiCurl_, CURLMOPT_SOCKETFUNCTION, socket_status_change_callback);
        curl_multi_setopt(multiCurl_, CURLMOPT_TIMERFUNCTION, multi_timer_cb);
        curl_multi_setopt(multiCurl_, CURLMOPT_MAXCONNECTS, 10000L);
        curl_multi_setopt(multiCurl_, CURLMOPT_MAX_HOST_CONNECTIONS, 100L);
        // todo:调整参数
        // curl_multi_setopt(multiCurl_, CURLMOPT_PIPELINING, CURLPIPE_HTTP1);
        // curl_multi_setopt(multiCurl_, CURLMOPT_MAX_PIPELINE_LENGTH, 100);
        std::thread([this]() {
            ev_loop(this->loop_, 0);
            ev_loop_destroy(this->loop_);
            // LOG_DEBUG << "loop ends";
        }).detach();
    }

    HttpClientProxy::~HttpClientProxy()
    {
        ev_timer_stop(loop_, &wakeupTimer_);
        ev_timer_stop(loop_, &taskTimer_);
        curl_multi_cleanup(multiCurl_);
        executor_->stop();
    }

    HttpResponse HttpClientProxy::getSync(const std::string & url, uint32_t timeoutMs)
    {
        ConnectionInfo connInfo;
        connInfo.url = url;
        CURL * curl = newCurl(&connInfo, verbose_);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeoutMs);
        CURLcode rc = curl_easy_perform(curl);
        HttpResponse response;
        if (rc == CURLE_OK) {
            int64_t responseCode;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
            response.status(responseCode);
            response.set_body(connInfo.dataBuffer.str());
        } else {
            if (rc == CURLE_OPERATION_TIMEDOUT) {
                LOG_WARN << "Curl Get " << url << " operation Timeout for " << timeoutMs << "ms";
            }
            response.status(500);
        }
        curl_easy_cleanup(curl);
        return response;
    }

    HttpResponse HttpClientProxy::postSync(const std::string & url, const std::string & postData,
                                           const std::string & contentType, uint32_t timeoutMs)
    {
        ConnectionInfo connInfo;
        connInfo.url = url;
        connInfo.postData = postData;
        CURL * curl = newCurl(&connInfo, verbose_);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeoutMs);
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, postData.length());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char *)connInfo.postData.c_str());
        curl_slist * headerList = nullptr;
        curl_slist_append(headerList, (std::string("Content-Type: ") + contentType).c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
        CURLcode rc = curl_easy_perform(curl);
        HttpResponse response;
        if (rc == CURLE_OK) {
            int64_t responseCode;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
            response.status(responseCode);
            response.set_body(connInfo.dataBuffer.str());
        } else {
            if (rc == CURLE_OPERATION_TIMEDOUT) {
                LOG_WARN << "Curl POST " << url << " operation Timeout for " << timeoutMs << "ms";
            }
            response.status(500);
        }
        curl_easy_cleanup(curl);
        return response;
    }

    future::PromisePtr HttpClientProxy::getAsync(const std::string & url, uint32_t timeoutMs,
                                                 const HttpResponseCallback & callback)
    {
        ConnectionInfo * connectionInfo = new ConnectionInfo();
        if (timeoutMs > 0) {
            connectionInfo->callbackInfo->deadline = time::getCurrentTimeStampMs() + timeoutMs;
        }
        connectionInfo->callbackInfo->transferFinishedCallback = callback;
        connectionInfo->url = url;
        CURL * curl = newCurl(connectionInfo, verbose_);
        connectionInfo->curl = curl;
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, timeout_checker);
        curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, connectionInfo);
        { //多线程环境,保证所有io in，timeout事件都在event loop线程环境内发生，否则会有并发问题
            std::lock_guard<std::mutex> lock(asyncCurlMutex_);
            pendingCurls_.push_back(curl);
            if (timeoutMs > 0) {
                timeoutList_.push(connectionInfo->callbackInfo);
            }
        }
        return connectionInfo->callbackInfo->promisePtr;
    }

    future::PromisePtr HttpClientProxy::postAsync(const std::string & url, const std::string & postData,
                                                  const std::string & contentType, uint32_t timeoutMs,
                                                  const HttpResponseCallback & callback)
    {
        ConnectionInfo * connectionInfo = new ConnectionInfo();
        connectionInfo->url = url;
        connectionInfo->postData = postData;
        if (timeoutMs > 0) {
            connectionInfo->callbackInfo->deadline = time::getCurrentTimeStampMs() + timeoutMs;
        }
        connectionInfo->callbackInfo->transferFinishedCallback = callback;
        CURL * curl = newCurl(connectionInfo, verbose_);
        connectionInfo->curl = curl;
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, timeout_checker);
        curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, connectionInfo);
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, postData.length());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char *)connectionInfo->postData.c_str());
        curl_slist * headerList = nullptr;
        curl_slist_append(headerList, (std::string("Content-Type: ") + contentType).c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
        {
            std::lock_guard<std::mutex> lock(asyncCurlMutex_);
            pendingCurls_.push_back(curl);
            if (timeoutMs > 0) {
                timeoutList_.push(connectionInfo->callbackInfo);
            }
        }
        return connectionInfo->callbackInfo->promisePtr;
    }
}
}
