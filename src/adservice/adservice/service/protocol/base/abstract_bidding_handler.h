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

    cppcms::json::value bannerJson2HttpsIOS(bool isIOS, const std::string & bannerJson, int bannerType);

    class BiddingFlowExtraInfo {
    public:
        int32_t mediaType;
        int32_t flowType;
        std::string keyWords;
        std::unordered_map<std::string, std::string> deviceIds;
        std::vector<std::string> dealIds;
        std::vector<int32_t> contentType;
        protocol::log::DeviceInfo devInfo;
        double feeRate{ 1.0 };
        std::string feerRateDetails;
    };

    class BidCookieMappingInfo {
    public:
        CookieMappingQueryKeyValue queryKV;
        MtUserMapping userMapping;
        bool needReMapping{ false };
        bool needTouchMapping{ false };
        bool needFixMapping{ false };
    };

    /**
     * BiddingHandler主要处理协议层数据转换,屏蔽各个平台之间输入输出的差异
     * 实际上应该叫做BiddingDataAdapter
     */
    class AbstractBiddingHandler {
    public:
        AbstractBiddingHandler()
            : isBidAccepted(false)
            , vastTemplateEngine("res/vast/vast.tpl")
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
         * @return 返回true需要记录日志，false不需要记录日志
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

        /**
         * 准备移动设备进行cookie mapping的key
         */
        const CookieMappingQueryKeyValue & cookieMappingKeyMobile(const std::string & idfa, const std::string & imei,
                                                                  const std::string & androidId,
                                                                  const std::string & mac,
                                                                  const AdSelectCondition & selectCondition,
                                                                  int appAdxId = 0, const std::string & appUserId = "");

        /**
         * 准备pc平台进行cookie mapping的key
         */
        const CookieMappingQueryKeyValue & cookieMappingKeyPC(int64_t adxId, const std::string & cookie);

        /**
         * 准备wap平台进行cookie mapping的key
         */
        const CookieMappingQueryKeyValue & cookieMappingKeyWap(int64_t adxId, const std::string & cookie)
        {
            return cookieMappingKeyPC(adxId, cookie);
        }

        /**
         * 查询cookie mapping
         */
        void queryCookieMapping(const CookieMappingQueryKeyValue & queryKV, AdSelectCondition & selectCondition);

        /**
         * 进行cookie mapping
         * @brief redoCookieMapping
         * @param adxId
         * @param adxCookieMappingUrl pc平台或wap平台用到的cookiemapping url,将在iframe中的image标签嵌入
         * @return
         */
        std::string redoCookieMapping(int64_t adxId, const std::string & adxCookieMappingUrl);

        const std::string & getShowBaseUrl(bool ssl);

        const std::string & getClickBaseUrl(bool ssl);

        inline bool bidFailedReturn()
        {
            return (isBidAccepted = false);
        }

        inline bool bidSuccessReturn()
        {
            return (isBidAccepted = true);
        }

    protected:
        void getShowPara(adservice::utility::url::URLHelper & url, const std::string & bid);

        void getClickPara(adservice::utility::url::URLHelper & url, const std::string & bid, const std::string & ref,
                          const std::string & landingUrl);

        std::string extractRealValue(const std::string & input, int adx);

        /**
         * search结束后，准备广告返回前调用，用于收集关键信息到adInfo
         */
        void fillAdInfo(const AdSelectCondition & selectCondition, const MT::common::SelectResult & result,
                        const std::string & adxUser);
        /**
         * 收集流量的额外信息到adFlowExtraInfo，目前主要是设备信息等
         */
        void buildFlowExtraInfo(const AdSelectCondition & selectCondition);

        std::string prepareVast(int width, int height, const cppcms::json::value & mtls, const std::string & tvm,
                                const std::string & cm);

    protected:
        //最近一次匹配的结果
        bool isBidAccepted;
        protocol::log::AdInfo adInfo;
        BidCookieMappingInfo cmInfo;
        BiddingFlowExtraInfo adFlowExtraInfo;
        adservice::utility::templateengine::TemplateEngine vastTemplateEngine;
    };
}
}

#endif // ADCORE_ABSTRACT_BIDDING_HANDLER_H
