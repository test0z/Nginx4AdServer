#ifndef __KUPAI_BIDDING_HANDLER_H__
#define __KUPAI_BIDDING_HANDLER_H__

#include "protocol/base/abstract_bidding_handler.h"
#include "utility/utility.h"
#include "wk_adx_rtb_ext.pb.h"
#include <map>
#include <tuple>

namespace protocol {
namespace bidding {

    typedef std::tuple<int, int, int> KupaiAdplaceStyle;

    class KupaiAdplaceStyleMap {
    public:
        KupaiAdplaceStyleMap()
        {
            add(4, 320, 50, 320, 50);
            add(4, 640, 100, 640, 100);
            add(5, 120, 120, 120, 120);
            add(4, 600, 500, 600, 500);
            add(4, 960, 640, 960, 640);
            add(4, 1080, 1920, 1080, 1920);
            add(4, 768, 1280, 768, 1280);
            add(4, 640, 960, 640, 960);
            add(2, 480, 360, 1, 0);
            add(3, 1000, 500, 2, 8);
            add(3, 1920, 1080, 2, 2);
            add(3, 600, 400, 2, 1);
            add(1, 480, 360, 3, 0);
        }
        inline void add(int style, int kw, int kh, int mw, int mh)
        {
            KupaiAdplaceStyle newStyle{ style, kw, kh };
            auto sizePair = std::make_pair(mw, mh);
            sizeMap.insert(std::make_pair(newStyle, sizePair));
            auto iter = sizeStyleMap.find(sizePair);
            if (iter == sizeStyleMap.end()) {
                std::vector<KupaiAdplaceStyle> styles{ newStyle };
                sizeStyleMap.insert({ sizePair, styles });
            } else {
                iter->second.push_back(newStyle);
            }
        }

        inline std::pair<int, int> getMttySize(int style, int w, int h)
        {
            KupaiAdplaceStyle k{ style, w, h };
            auto iter = sizeMap.find(k);
            if (iter == sizeMap.end()) {
                return std::make_pair(w, h);
            } else {
                return iter->second;
            }
        }

        const std::unordered_map<std::pair<int, int>, std::vector<KupaiAdplaceStyle>> & getSizeStyleMap() const
        {
            return sizeStyleMap;
        }

    private:
        //我方尺寸到kupai的映射
        std::unordered_map<std::pair<int, int>, std::vector<KupaiAdplaceStyle>> sizeStyleMap;
        // kupai 尺寸到 我方尺寸的映射
        std::unordered_map<KupaiAdplaceStyle, std::pair<int, int>> sizeMap;
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
