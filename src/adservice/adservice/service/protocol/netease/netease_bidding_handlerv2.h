//
// Created by guoze.lin on 16/6/27.
//

#ifndef ADCORE_NETEASE_BIDDING_HANDLER_H
#define ADCORE_NETEASE_BIDDING_HANDLER_H

#include <map>

#include "protocol/base/abstract_bidding_handler.h"
#include "utility/utility.h"
#include <unordered_set>

namespace protocol {
namespace bidding {

    struct NetEaseAdplaceStyle {
        std::unordered_set<std::pair<int, int>> sizes;
        NetEaseAdplaceStyle()
        {
        }
        NetEaseAdplaceStyle(int w, int h)
        {
            sizes.insert({ w, h });
        }
        void addSize(int w, int h)
        {
            sizes.insert({ w, h });
        }
    };

    class NetEaseAdplaceStyleMap {
    public:
        NetEaseAdplaceStyleMap()
        {
            add(3, 1, 0);
            add(10, 2, 0);
            add(10, 2, 12);
            add(11, 3, 0);
            add(13, 4, 0);
        }
        inline void add(int key, int width, int height)
        {
            auto iter = styleMap.find(key);
            if (iter == styleMap.end()) {
                styleMap.insert(std::make_pair(key, NetEaseAdplaceStyle(width, height)));
            } else {
                auto & style = iter->second;
                style.addSize(width, height);
            }
            sizeStyleMap.insert(std::make_pair(std::make_pair(width, height), key));
        }
        inline NetEaseAdplaceStyle & get(int key)
        {
            return styleMap[key];
        }
        inline bool find(int key)
        {
            return styleMap.find(key) != styleMap.end();
        }

        const std::unordered_map<std::pair<int, int>, int> & getSizeStyleMap() const
        {
            return sizeStyleMap;
        }

    private:
        std::map<int, NetEaseAdplaceStyle> styleMap;
        std::unordered_map<std::pair<int, int>, int> sizeStyleMap;
    };

    class NetEaseBiddingHandler : public AbstractBiddingHandler {
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
        cppcms::json::value bidRequest;
        cppcms::json::value bidResponse;
    };
}
}

#endif // ADCORE_NETEASE_BIDDING_HANDLER_H
