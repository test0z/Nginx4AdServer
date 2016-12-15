#ifndef __KUPAI_BIDDING_HANDLER_H__
#define __KUPAI_BIDDING_HANDLER_H__

#include "protocol/base/abstract_bidding_handler.h"
#include "utility/utility.h"
#include "wk_adx_rtb_ext.pb.h"
#include <map>

namespace protocol {
namespace bidding {

    struct KupaiAdplaceStyle {
        int width, height;
        KupaiAdplaceStyle()
        {
        }
        KupaiAdplaceStyle(int w, int h)
            : width(w)
            , height(h)
        {
        }
    };

    class KupaiAdplaceStyleMap {
    public:
        KupaiAdplaceStyleMap()
        {
            add(4, 320, 50);
            add(4, 640, 100);
            add(5, 120, 120);
            add(4, 600, 500);
            add(4, 960, 640);
            add(4, 1080, 1920);
            add(4, 768, 1280);
            add(4, 640, 960);
            add(2, 1, 1);
            add(2, 1, 0);
            add(3, 2, 7);
            add(3, 2, 2);
            add(3, 2, 8);
            add(1, 3, 0);
        }
        inline void add(int key, int width, int height)
        {
            styleMap.insert(std::make_pair(key, KupaiAdplaceStyle(width, height)));
            sizeStyleMap.insert(std::make_pair(std::make_pair(width, height), key));
        }
        inline KupaiAdplaceStyle & get(int key)
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
        std::map<int, KupaiAdplaceStyle> styleMap;
        std::unordered_map<std::pair<int, int>, int> sizeStyleMap;
    };

    class KupaiBiddingHandler : public AbstractBiddingHandler {
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
        com::google::openrtb::BidRequest bidRequest_;
        com::google::openrtb::BidResponse bidResponse_;
    };

} // namespace bidding
} // namespace protocol

#endif // __KUPAI_BIDDING_HANDLER_H__
