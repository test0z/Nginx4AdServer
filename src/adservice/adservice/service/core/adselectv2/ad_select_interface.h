#ifndef AD_SELECT_INTERFACE_H
#define AD_SELECT_INTERFACE_H

#include <ostream>
#include <string>
#include <vector>

#include <mtty/constants.h>

namespace adservice {
namespace adselectv2 {

    /**
     * 在查询过程中的预设广告位信息
     */
    class PreSetAdplaceInfo {
    public:
        std::vector<std::pair<int64_t, int64_t>> sizeArray;
        int flowType{ 0 };
    };

    template <typename CharT, typename TraitsT>
    std::basic_ostream<CharT, TraitsT> operator<<(std::basic_ostream<CharT, TraitsT> & os,
                                                  const PreSetAdplaceInfo & info)
    {
        os << "presetflowtype:" << info.flowType << ",";
        for (auto & t : info.sizeArray) {
            os << "size:" << t.first << "x" << t.second << ",";
        }

        return os;
    }

    struct AdSelectCondition {
        // adx平台广告位ID
        std::string adxpid;
        // mtty内部广告位ID
        int64_t mttyPid{ 0 };
        //需要匹配的ip地址
        std::string ip;
        //需要匹配的分时编码
        std::string dHour;
        //需要匹配的移动设备过滤条件字符串表示
        std::string mobileDeviceStr;
        //需要匹配的pc端浏览器类型过滤条件
        std::string pcBrowserStr;
        //需要匹配的deal,比如优酷的dealid
        std::string dealId;
        //流量底价
        int basePrice{ 0 };
        //需要匹配的adxid
        int adxid{ 0 };
        //需要匹配的地域编码
        int dGeo{ 0 };
        //需要匹配的banner的宽
        int width{ 0 };
        //需要匹配的banner的高
        int height{ 0 };
        //需要匹配的媒体类型,编码为mtty定义的媒体类型
        int mediaType{ 0 };
        //需要匹配的广告位类型,编码为mtty定义的广告位类型
        int adplaceType{ 0 };
        //需要匹配的流量类型,编码为mtty定义的流量类型
        int flowType{ 0 };
        //需要匹配的屏数
        int displayNumber{ 0 };
        //需要匹配的pc端操作系统类型
        int pcOS{ 0 };
        //需要匹配的移动端设备类型
        int mobileDevice{ 0 };
        //需要匹配的网络类型
        int mobileNetwork{ 0 };
        //需要匹配的运营商类型
        int mobileNetWorkProvider{ 0 };
        //需要匹配的内容类型
        int mttyContentType{ 0 };
        //需要匹配的banner 类型
        int bannerType{ 0 };
        //提供的mt 用户id，可以用于查找频次信息
        std::string mtUserId;
        // 预设的广告位信息,比如从ADX流量获取的信息填充到这里,可以省略在ES中对广告位的查询
        PreSetAdplaceInfo * pAdplaceInfo{ nullptr };
    };

    template <typename CharT, typename TraitsT>
    std::basic_ostream<CharT, TraitsT> operator<<(std::basic_ostream<CharT, TraitsT> & os,
                                                  const AdSelectCondition & condition)
    {
        os << "adxpid:" << condition.adxpid << ",";
        os << "mttypid:" << condition.mttyPid << ",";
        os << "ip:" << condition.ip << ",";
        os << "dHour:" << condition.dHour << ",";
        os << "adxid:" << condition.adxid << ",";
        os << "dGeo:" << condition.dGeo << ",";
        os << "width:" << condition.width << ",";
        os << "height:" << condition.height << ",";
        os << "mediaType:" << condition.mediaType << ",";
        os << "adplaceType:" << condition.adplaceType << ",";
        os << "flowType:" << condition.flowType << ",";
        os << "displayNumber:" << condition.displayNumber << ",";
        os << "pcBrowser:" << condition.pcBrowserStr << ",";
        os << "pcOs:" << condition.pcOS << ",";
        os << "contenttype:" << condition.mttyContentType << ",";
        os << "mobileDevice:" << condition.mobileDevice << ",";
        os << "mobileNetwork:" << condition.mobileNetwork << ",";
        os << "mobileNetworkProvider:" << condition.mobileNetWorkProvider << ",";
        os << "bannerType:" << condition.bannerType << ",";
        os << "dealId:" << condition.dealId << ",";
        os << "mtUserId:" << condition.mtUserId;
        if (condition.pAdplaceInfo != nullptr) {
            os << "," << *condition.pAdplaceInfo;
        }

        return os;
    }
}
}

#endif // AD_SELECT_INTERFACE_H
