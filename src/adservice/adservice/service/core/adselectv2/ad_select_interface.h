#ifndef AD_SELECT_INTERFACE_H
#define AD_SELECT_INTERFACE_H

#include <vector>
#include <tuple>
#include <string>
#include <sstream>
#include <boost/variant.hpp>
#include <unordered_map>
#include <boost/serialization/access.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/unordered_map.hpp>

#include "common/constants.h"
#include "addata.h"
namespace adservice{
namespace adselectv2{

typedef std::array<boost::variant<double, int64_t, std::string>, 2> TupleType;
typedef boost::variant<double, int64_t, std::string, TupleType> ListItemType;
typedef std::vector<ListItemType> ListType;
typedef boost::variant<double, int64_t, std::string, TupleType, ListType> ValueType;
typedef std::unordered_map<std::string,ValueType> Source;

class LogicParam{
public:
    bool fromSSP{false};
    int mttyPid{0};
    int adxId{0};
    std::string adxPid{""};
private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & fromSSP;
        ar & mttyPid;
        ar & adxId;
        ar & adxPid;
    }
};

class AdSelectResult{
public:
    AdSolution solution;
    AdBanner banner;
    AdAdplace adplace;
    int bidPrice;
    int ppid;
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & solution;
        ar & banner;
        ar & adplace;
        ar & bidPrice;
        ar & ppid;
    }
};

class AdSelectRequest{
public:
    Source source;
    LogicParam param;
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & source;
        ar & param;
    }
};

class AdSelectResponse{
public:
    AdSelectResult result;
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & result;
    }
};

/**
 * 在查询过程中的预设广告位信息
 */
struct PreSetAdplaceInfo{
    std::vector<std::tuple<int,int>> sizeArray;
    int flowType;
    PreSetAdplaceInfo(){
        flowType  = 0;
    }
    std::string toString() const{
        std::stringstream ss;
        ss<<"presetflowtype:"<<flowType<<",";
        for(auto t : sizeArray){
            ss<<"size:"<<std::get<0>(t)<<"x"<<std::get<1>(t)<<",";
        }
        return ss.str();
    }
};

struct AdSelectCondition {
    //adx平台广告位ID
    std::string adxpid;
    //mtty内部广告位ID
    int64_t mttyPid;
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
    int basePrice;
    //需要匹配的adxid
    int adxid;
    //需要匹配的地域编码
    int dGeo;
    //需要匹配的banner的宽
    int width;
    //需要匹配的banner的高
    int height;
    //需要匹配的媒体类型,编码为mtty定义的媒体类型
    int mediaType;
    //需要匹配的广告位类型,编码为mtty定义的广告位类型
    int adplaceType;
    //需要匹配的流量类型,编码为mtty定义的流量类型
    int flowType;
    //需要匹配的屏数
    int displayNumber;
    //需要匹配的pc端操作系统类型
    int pcOS;
    //需要匹配的移动端设备类型
    int mobileDevice;
    //需要匹配的网络类型
    int mobileNetwork;
    //需要匹配的运营商类型
    int mobileNetWorkProvider;
    //需要匹配的内容类型
    int mttyContentType;
    // 预设的广告位信息,比如从ADX流量获取的信息填充到这里,可以省略在ES中对广告位的查询
    PreSetAdplaceInfo* pAdplaceInfo;

    AdSelectCondition(){
        mttyPid = 0;
        dGeo = 0;
        width = 0;
        height = 0;
        mediaType = 0;
        adplaceType = 0;
        flowType = SOLUTION_FLOWTYPE_UNKNOWN;
        displayNumber = 0;
        pcOS = 0;
        mobileDevice = 0;
        mobileNetwork = 0;
        mobileNetWorkProvider = 0;
        basePrice = 0;
        pAdplaceInfo = NULL;
        dealId = "0";
    }

    std::string toString() const {
        std::stringstream ss;
        ss<<"adxpid:"<<adxpid<<",";
        ss<<"mttypid:"<<mttyPid<<",";
        ss<<"ip:"<<ip<<",";
        ss<<"dHour:"<<dHour<<",";
        ss<<"adxid:"<<adxid<<",";
        ss<<"dGeo:"<<dGeo<<",";
        ss<<"width:"<<width<<",";
        ss<<"height:"<<height<<",";
        ss<<"mediaType:"<<mediaType<<",";
        ss<<"adplaceType:"<<adplaceType<<",";
        ss<<"flowType:"<<flowType<<",";
        ss<<"displayNumber:"<<displayNumber<<",";
        ss<<"pcBrowser:"<<pcBrowserStr<<",";
        ss<<"pcOs:"<<pcOS<<",";
        ss<<"contenttype:"<<mttyContentType<<",";
        ss<<"mobileDevice:"<<mobileDevice<<",";
        ss<<"mobileNetwork:"<<mobileNetwork<<",";
        ss<<"mobileNetworkProvider:"<<mobileNetWorkProvider<<",";
        ss<<"dealId:"<<dealId;
        if(pAdplaceInfo!=NULL){
            ss<<","<<pAdplaceInfo->toString();
        }
        return ss.str();
    }

};

}
}

#endif // AD_SELECT_INTERFACE_H
