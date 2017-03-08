//
// Created by guoze.lin on 16/5/3.
//

#ifndef ADCORE_ABSTRACT_BIDDING_HANDLER_H
#define ADCORE_ABSTRACT_BIDDING_HANDLER_H

#include <functional>
#include <mtty/types.h>
#include <string>
#include <vector>

#include "common/functions.h"
#include "common/types.h"
#include "core/adselectv2/ad_select_interface.h"
#include "core/core_cm_manager.h"
#include "core/model/user.h"
#include "protocol/log/log.h"
#include "utility/utility.h"
#include <mtty/constants.h>

namespace protocol {
namespace bidding {

    using namespace adservice::adselectv2;
    using namespace adservice::server;
    using namespace adservice::core::model;

    class AbstractBiddingHandler;

    typedef std::function<bool(AbstractBiddingHandler *, std::vector<AdSelectCondition> &)> BiddingFilterCallback;

    void extractSize(const std::string & size, int & width, int & height);
    std::string makeBidSize(int width, int height);

    inline void urlHttp2HttpsIOS(bool isIOS, std::string & url)
    {
        if (isIOS) {
            adservice::utility::url::url_replace_all(url, "http://", "https://");
        }
    }

    class BiddingFlowExtraInfo {
    public:
        int32_t mediaType;
        int32_t flowType;
        std::string keyWords;
        std::string deviceIdName;
        std::vector<std::string> dealIds;
        std::vector<int32_t> contentType;
        protocol::log::DeviceInfo devInfo;
    };

    class BidCookieMappingInfo {
    public:
        CookieMappingQueryKeyValue queryKV;
        MtUserMapping userMapping;
        bool needReMapping{ false };
    };

    /**
     * BiddingHandler主要处理协议层数据转换,屏蔽各个平台之间输入输出的差异
     * 实际上应该叫做BiddingDataAdapter
     */
    class AbstractBiddingHandler {
    public:
        AbstractBiddingHandler()
            : isBidAccepted(false)
        {
        }

        virtual ~AbstractBiddingHandler()
        {
        }

        /**
         * 从Adx Bid Post请求数据中获取具体的请求信息
         */
        virtual bool parseRequestData(const std::string & data) = 0;

        /**
         * 根据Bid 的相关信息对日志进行信息填充
         */
        virtual bool fillLogItem(const AdSelectCondition & selectCondition, protocol::log::LogItem & logItem,
                                 bool isAccepted = false);

        /**
         * @brief fillSpecificLog 各家平台具体日志字段的标准
         * @param isAccepted
         * @return
         */
        virtual bool fillSpecificLog(const AdSelectCondition & selectCondition, protocol::log::LogItem & logItem,
                                     bool isAccepted = false)
        {
            return true;
        }

        /**
         * 根据ADX的请求进行竞价匹配,决定是否接受这个流量,同时设置isBidAccepted
         * @return: true接受流量,false不接受流量
         */
        virtual bool filter(const BiddingFilterCallback & filterCb)
        {
            isBidAccepted = false;
            return false;
        }

        /**
         * 将匹配结果转换为具体平台的格式的结果
         */
        virtual void buildBidResult(const AdSelectCondition & selectCondition, const MT::common::SelectResult & result,
                                    int seq = 0)
            = 0;

        /**
         * 当接受流量时装配合适的输出
         */
        virtual void match(INOUT adservice::utility::HttpResponse & response) = 0;

        /**
         * 不接受ADX的流量请求
         */
        virtual void reject(INOUT adservice::utility::HttpResponse & response) = 0;

        /**
         * 产生htmlsnippet
         */
        virtual std::string generateHtmlSnippet(const std::string & bid, int width, int height, const char * extShowBuf,
                                                const char * cookieMappingUrl = "", bool useHttps = false);

        virtual std::string generateScript(const std::string & bid, int width, int height, const char * scriptUrl,
                                           const char * clickMacro, const char * extParam);

        const CookieMappingQueryKeyValue & cookieMappingKeyMobile(const std::string & idfa, const std::string & imei,
                                                                  const std::string & androidId,
                                                                  const std::string & mac);

        const CookieMappingQueryKeyValue & cookieMappingKeyPC(int64_t adxId, const std::string & cookie);

        const CookieMappingQueryKeyValue & cookieMappingKeyWap(int64_t adxId, const std::string & cookie)
        {
            return cookieMappingKeyPC(adxId, cookie);
        }

        void queryCookieMapping(const CookieMappingQueryKeyValue & queryKV, AdSelectCondition & selectCondition);

        std::string redoCookieMapping(int64_t adxId, const std::string & adxCookieMappingUrl);

        inline bool bidFailedReturn()
        {
            return (isBidAccepted = false);
        }

        inline bool bidSuccessReturn()
        {
            return (isBidAccepted = true);
        }

    protected:
        void getShowPara(const std::string & bid, char * showParamBuf, int showBufSize);
        void getShowPara(adservice::utility::url::URLHelper & url, const std::string & bid);
        void getClickPara(const std::string & bid, char * clickParamBuf, int clickBufSize, const std::string & ref,
                          const std::string & landingurl);
        void getClickPara(adservice::utility::url::URLHelper & url, const std::string & bid, const std::string & ref,
                          const std::string & landingUrl);
        int extractRealValue(const std::string & input, int adx);

        void fillAdInfo(const AdSelectCondition & selectCondition, const MT::common::SelectResult & result,
                        const std::string & adxUser);

        void buildFlowExtraInfo(const AdSelectCondition & selectCondition);

    protected:
        //最近一次匹配的结果
        bool isBidAccepted;
        protocol::log::AdInfo adInfo;
        BidCookieMappingInfo cmInfo;
        BiddingFlowExtraInfo adFlowExtraInfo;
    };
}
}

#endif // ADCORE_ABSTRACT_BIDDING_HANDLER_H
