//
// Created by guoze.lin on 16/6/27.
//

#ifndef ADCORE_NETEASE_BIDDING_HANDLER_H
#define ADCORE_NETEASE_BIDDING_HANDLER_H

#include <map>

#include "protocol/base/abstract_bidding_handler.h"
#include "utility/json.h"

namespace std {

template <>
struct hash<std::pair<int, int>> {
    size_t operator()(const std::pair<int, int> & arg) const noexcept
    {
        auto h = std::hash<int>();
        return h(arg.first) ^ h(arg.second);
    }
};
}

namespace protocol {
namespace bidding {

    struct NetEaseAdplaceStyle {
        int width, height;
        NetEaseAdplaceStyle()
        {
        }
        NetEaseAdplaceStyle(int w, int h)
            : width(w)
            , height(h)
        {
        }
    };

    class NetEaseAdplaceStyleMap {
    public:
        NetEaseAdplaceStyleMap()
        {
            add(3, 1, 0);
            add(10, 2, 0);
            add(11, 3, 0);
        }
        inline void add(int key, int width, int height)
        {
            styleMap.insert(std::make_pair(key, NetEaseAdplaceStyle(width, height)));
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
