//
// Created by guoze.lin on 16/6/27.
//

#ifndef ADCORE_SOHU_BIDDING_HANDLER_H
#define ADCORE_SOHU_BIDDING_HANDLER_H

#include "protocol/base/abstract_bidding_handler.h"
#include "protocol/sohu/sohuRTB.pb.h"

namespace protocol {
namespace bidding {

    class SohuSizeMap {
    public:
        SohuSizeMap()
        {
            add(std::make_pair(270, 202), std::make_pair(1, 0));
            add(std::make_pair(240, 180), std::make_pair(1, 1));
            add(std::make_pair(984, 328), std::make_pair(2, 0));
            add(std::make_pair(1200, 800), std::make_pair(2, 1));
            add(std::make_pair(1280, 720), std::make_pair(2, 2));
            add(std::make_pair(1200, 627), std::make_pair(2, 3));
            add(std::make_pair(800, 1200), std::make_pair(2, 4));
            add(std::make_pair(640, 288), std::make_pair(2, 5));
            add(std::make_pair(1000, 560), std::make_pair(2, 6));
            add(std::make_pair(270, 202), std::make_pair(3, 0));
            add(std::make_pair(480, 240), std::make_pair(2, 7));
            add(std::make_pair(640, 320), std::make_pair(2, 8));
            add(std::make_pair(1080, 540), std::make_pair(2, 9));
            add(std::make_pair(360, 234), std::make_pair(1, 2));
            add(std::make_pair(656, 324), std::make_pair(2, 10));
        }
        void add(const std::pair<int, int> & k, const std::pair<int, int> & v)
        {
            sizemap.insert(std::make_pair(k, v));
        }

        std::pair<int, int> get(const std::pair<int, int> & k)
        {
            if (sizemap.find(k) != sizemap.end()) {
                return sizemap[k];
            } else {
                return k;
            }
        }

    private:
        std::map<std::pair<int, int>, std::pair<int, int>> sizemap;
    };

    class SohuBiddingHandler : public AbstractBiddingHandler {
    public:
        /**
         * 从Adx Bid Post请求数据中获取具体的请求信息
         */
        bool parseRequestData(const std::string & data);

        /**
         * 根据Bid 的相关信息对日志进行信息填充
         */
        bool fillLogItem(protocol::log::LogItem & logItem);

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
