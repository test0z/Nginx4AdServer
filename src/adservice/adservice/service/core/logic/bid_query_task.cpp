//
// Created by guoze.lin on 16/5/3.
//

#include "bid_query_task.h"
#include "common/atomic.h"
#include "protocol/baidu/baidu_bidding_handler.h"
#include "protocol/guangyin/guangyin_bidding_handler.h"
#include "protocol/netease/netease_bidding_handlerv2.h"
#include "protocol/sohu/sohu_bidding_handler.h"
#include "protocol/tanx/tanx_bidding_handler.h"
#include "protocol/tencent_gdt/gdt_bidding_handler.h"
#include "protocol/youku/youku_bidding_handlerv2.h"
#include "utility/utility.h"

#include "core/adselectv2/ad_select_client.h"
#include "logging.h"

extern adservice::adselectv2::AdSelectClientPtr adSelectClient;

namespace adservice {
namespace corelogic {

    using namespace adservice;
    using namespace adservice::utility;
    using namespace adservice::utility::time;
    using namespace adservice::server;
    using namespace adservice::adselectv2;

    int HandleBidQueryTask::initialized = 0;
    int HandleBidQueryTask::moduleCnt = 0;
    int HandleBidQueryTask::moduleIdx[BID_MAX_MODULES];
    struct ModuleIndex HandleBidQueryTask::moduleAdx[BID_MAX_MODULES];

    static int handleBidRequests = 0;
    static int updateBidRequestsTime = 0;

#define ADD_MODULE_ENTRY(name, adxid)                                     \
    {                                                                     \
        moduleIdx[moduleCnt] = moduleCnt;                                 \
        moduleAdx[moduleCnt++] = { fnv_hash(name, strlen(name)), adxid }; \
    }
    /**
             * 初始化不同ADX的请求模块索引表
             */
    void HandleBidQueryTask::init()
    {
        if (initialized == 1 || !ATOM_CAS(&initialized, 0, 1))
            return;
        moduleCnt = 0;

        //百度ADX
        ADD_MODULE_ENTRY(BID_QUERY_PATH_BAIDU, ADX_BAIDU);
        //淘宝ADX
        ADD_MODULE_ENTRY(BID_QUERY_PATH_TANX, ADX_TANX);
        //优酷ADX
        ADD_MODULE_ENTRY(BID_QUERY_PATH_YOUKU, ADX_YOUKU);
        //腾讯ADX
        ADD_MODULE_ENTRY(BID_QUERY_PATH_GDT, ADX_TENCENT_GDT);
        //搜狐ADX
        ADD_MODULE_ENTRY(BID_QUERY_PATH_SOHU, ADX_SOHU_PC);
        //网易客户端ADX
        ADD_MODULE_ENTRY(BID_QUERY_PATH_NETEASE, ADX_NETEASE_MOBILE);
        //光阴网络ADX
        ADD_MODULE_ENTRY(BID_QUERY_PATH_GUANGYIN, ADX_GUANGYIN);

        std::sort<int *>(moduleIdx, moduleIdx + moduleCnt, [](const int & a, const int & b) {
            return moduleAdx[a].moduleHash < moduleAdx[b].moduleHash;
        });
    }

    /**
             * 根据模块路径获取对应的Adx
             */
    int HandleBidQueryTask::getAdxId(const std::string & path)
    {
        auto key = fnv_hash(path.c_str(), path.length());
        int l = 0, h = moduleCnt - 1;
        while (l <= h) {
            int mid = l + ((h - l) >> 1);
            if (key <= moduleAdx[moduleIdx[mid]].moduleHash)
                h = mid - 1;
            else
                l = mid + 1;
        }
        if (l < 0 || l >= moduleCnt || moduleAdx[moduleIdx[l]].moduleHash != key) {
            return 0;
        }
        return moduleAdx[moduleIdx[l]].adxId;
    }

    AbstractBiddingHandler * HandleBidQueryTask::getBiddingHandler(int adxId)
    {
        switch (adxId) {
        case ADX_TANX:
            return new TanxBiddingHandler();
        case ADX_YOUKU:
            return new YoukuBiddingHandler();
        case ADX_BAIDU:
            return new BaiduBiddingHandler();
        case ADX_TENCENT_GDT:
            return new GdtBiddingHandler();
        case ADX_SOHU_PC:
            return new SohuBiddingHandler();
        case ADX_NETEASE_MOBILE:
            return new NetEaseBiddingHandler();
        case ADX_GUANGYIN:
            return new GuangyinBiddingHandler();
        default:
            return NULL;
        }
    }

    void HandleBidQueryTask::updateBiddingHandler()
    {
        if (biddingHandler == NULL) {
            BidThreadLocal & bidData = threadData->bidData;
            BiddingHandlerMap::iterator iter;
            if ((iter = bidData.biddingHandlers.find(adxId)) == bidData.biddingHandlers.end()) {
                biddingHandler = getBiddingHandler(adxId);
                bidData.biddingHandlers.insert(std::make_pair(adxId, biddingHandler));
            } else {
                biddingHandler = iter->second;
            }
        }
    }

    void HandleBidQueryTask::getPostParam(ParamMap & paramMap)
    {
        updateBiddingHandler();
        if (biddingHandler == NULL) {
            LOG_WARN << "Bidding Handler Not Found,adxId:" << adxId;
            return;
        }
        if (!biddingHandler->parseRequestData(data)) {
            LOG_WARN << "Parse Bidding Request Failed,adxId" << adxId;
        }
    }

    void HandleBidQueryTask::customLogic(ParamMap & paramMap, protocol::log::LogItem & log,
                                         adservice::utility::HttpResponse & resp)
    {
        updateBiddingHandler();
        if (!isPost || biddingHandler == NULL) {
            log.reqStatus = 500;
            resp.status(200);
        } else {
            TaskThreadLocal * localData = threadData;
            bool bidResult = biddingHandler->filter(
                [localData, &log](AbstractBiddingHandler * adapter, adselectv2::AdSelectCondition & condition) -> bool {
                    int seqId = 0;
                    seqId = localData->seqId;
                    //地域定向接入
                    IpManager & ipManager = IpManager::getInstance();
                    condition.dGeo = ipManager.getAreaByIp(condition.ip.data());
                    condition.dHour = adSelectTimeCodeUtc();
                    MT::common::SelectResult resp;
                    if (!adSelectClient->search(seqId, false, condition, resp)) {
                        condition.mttyPid = resp.adplace.pId;
                        log.adInfo.mid = resp.adplace.mId;
                        return false;
                    }
                    adapter->buildBidResult(condition, resp);
                    log.adInfo.mid = resp.adplace.mId;
                    return true;
                });
            if (bidResult) {
                biddingHandler->match(resp);
            } else {
                biddingHandler->reject(resp);
            }
            if (!biddingHandler->fillLogItem(log)) {
                needLog = false;
            }
        }
        handleBidRequests++;
        if (handleBidRequests % 10000 == 1) {
            int64_t todayStartTime = time::getTodayStartTime();
            if (updateBidRequestsTime < todayStartTime) {
                handleBidRequests = 1;
                updateBidRequestsTime = todayStartTime;
            } else {
                LOG_INFO << "handleBidRequests:" << handleBidRequests;
            }
        }
    }

    void HandleBidQueryTask::onError(std::exception & e, adservice::utility::HttpResponse & resp)
    {
        std::stringstream ss;
        ss << booster::trace(e);

        LOG_ERROR << "error occured in HandleBidQueryTask:" << e.what() << ",query:" << data
                  << ",backtrace:" << ss.str();
    }
}
}
