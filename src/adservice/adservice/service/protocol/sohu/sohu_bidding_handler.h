//
// Created by guoze.lin on 16/6/27.
//

#ifndef ADCORE_SOHU_BIDDING_HANDLER_H
#define ADCORE_SOHU_BIDDING_HANDLER_H

#include "protocol/base/abstract_bidding_handler.h"
#include "protocol/sohu/sohuRTB.pb.h"

namespace protocol {
namespace bidding {

    class SohuBiddingHandler : public AbstractBiddingHandler {
    public:
        /**
         * 从Adx Bid Post请求数据中获取具体的请求信息
         */
        bool parseRequestData(const std::string & data);

        /**
         * 根据Bid 的相关信息对日志进行信息填充
         */
        bool fillLogItem(protocol::log::LogItem & logItem, bool isAccepted);

        /**
         * 根据ADX的请求进行竞价匹配,决定是否接受这个流量,同时设置isBidAccepted
         * @return: true接受流量,false不接受流量
         */
        bool filter(const BiddingFilterCallback & filterCb);

        /**
         * 将匹配结果转换为具体平台的格式的结果
         */
        void buildBidResult(const AdSelectCondition & queryCondition, const MT::common::SelectResult & result);

        /**
         * 当接受流量时装配合适的输出
         */
        void match(INOUT adservice::utility::HttpResponse & response);

        /**
         * 不接受ADX的流量请求
         */
        void reject(INOUT adservice::utility::HttpResponse & response);

    private:
        std::string getDisplayPara();
        std::string getSohuClickPara(const std::string & url);

    private:
        protocol::sohuadx::Request bidRequest;
        protocol::sohuadx::Response bidResponse;
    };
}
}

#endif // ADCORE_SOHU_BIDDING_HANDLER_H
