#ifndef YIDIAN_BIDDING_HANDLER_H
#define YIDIAN_BIDDING_HANDLER_H

#include "protocol/base/abstract_bidding_handler.h"
#include "utility/json.h"

namespace protocol {
namespace bidding {
    class YidianBidingHandler : public AbstractBiddingHandler {
    public:
        YidianBidingHandler()
            : Adtype(-1)
        {
        }
        bool filter(const BiddingFilterCallback & filterCb);
        /**
         * @brief fillSpecificLog 各家平台具体日志字段的标准
         * @param isAccepted
         * @return
         */
        bool fillSpecificLog(const AdSelectCondition & selectCondition, protocol::log::LogItem & logItem,
                             bool isAccepted = false);

        /**
         * 将匹配结果转换为具体平台的格式的结果
         */
        void buildBidResult(const AdSelectCondition & queryCondition, const MT::common::SelectResult & result, int seq);
        bool parseRequestData(const std::string & data);
        /**
         * 当接受流量时装配合适的输出
         */
        void match(INOUT adservice::utility::HttpResponse & response);

        /**
         * 不接受ADX的流量请求
         */
        void reject(INOUT adservice::utility::HttpResponse & response);

    private:
        cppcms::json::value bidRequest;
        cppcms::json::value bidResponse;
        int Adtype;
        int maxduration;
    };
}
}
#endif
