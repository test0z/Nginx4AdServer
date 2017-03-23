//
// Created by guoze.lin on 16/5/3.
//

#ifndef ADCORE_BAIDU_BIDDING_HANDLER_H
#define ADCORE_BAIDU_BIDDING_HANDLER_H

#include "protocol/base/abstract_bidding_handler.h"
#include "protocol/m_baidu/baidu_bidding.h"

namespace protocol {
namespace bidding {

    class BaiduBiddingHandler : public AbstractBiddingHandler {
    public:
        /**
         * 从Adx Bid Post请求数据中获取具体的请求信息
         */
        bool parseRequestData(const std::string & data);

        /**
         * @brief fillSpecificLog 各家平台具体日志字段的标准
         * @param isAccepted
         * @return
         */
        bool fillSpecificLog(const AdSelectCondition & selectCondition, protocol::log::LogItem & logItem,
                             bool isAccepted = false);

        /**
         * 根据ADX的请求进行竞价匹配,决定是否接受这个流量,同时设置isBidAccepted
         * @return: true接受流量,false不接受流量
         */
        bool filter(const BiddingFilterCallback & filterCb);

        /**
         * 将匹配结果转换为具体平台的格式的结果
         */
        void buildBidResult(const AdSelectCondition & queryCondition, const MT::common::SelectResult & result,
                            int seq = 0);

        /**
         * 当接受流量时装配合适的输出
         */
        void match(INOUT adservice::utility::HttpResponse & response);

        /**
         * 不接受ADX的流量请求
         */
        void reject(INOUT adservice::utility::HttpResponse & response);

    private:
        std::string baiduHtmlSnippet(const std::string & cookieMappingUrl, bool useHttps);
        std::string generateHtmlSnippet(const std::string & bid, int width, int height, const char * extShowBuf,
                                        const char * cookieMappingUrl, bool useHttps);
        std::string baiduHtmlScript();

    private:
        protocol::baidu::BidRequest bidRequest;
        protocol::baidu::BidResponse bidResponse;
    };
}
}

#endif // ADCORE_BAIDU_BIDDING_HANDLER_H
