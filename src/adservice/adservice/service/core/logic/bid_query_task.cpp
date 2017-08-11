//
// Created by guoze.lin on 16/5/3.
//

#include "bid_query_task.h"

#include "common/atomic.h"
#include "protocol/2345/adx2345_bidding_handler.h"
#include "protocol/360/360max_bidding_handler.h"
#include "protocol/baidu/baidu_bidding_handler.h"
#include "protocol/guangyin/guangyin_bidding_handler.h"
#include "protocol/inmobi/inmobi_bidding_handler.h"
#include "protocol/kupai/kupai_bidding_handler.h"
#include "protocol/liebao/liebao_bidding_handler.h"
#include "protocol/netease/netease_bidding_handlerv2.h"
#include "protocol/netease/nex_bidding_handler.h"
#include "protocol/sohu/sohu_bidding_handler.h"
#include "protocol/tanx/tanx_bidding_handler.h"
#include "protocol/tencent_gdt/gdt_bidding_handler.h"
#include "protocol/yidian/yidian_bidding_handler.h"
#include "protocol/youku/youku_bidding_handlerv2.h"

#include "core/adselectv2/ad_select_client.h"
#include "core/core_scenario_manager.h"
#include "logging.h"
#include "utility/utility.h"

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
        // inmobi ADX
        ADD_MODULE_ENTRY(BID_QUERY_PATH_INMOBI, ADX_INMOBI);
        //网易NEX
        ADD_MODULE_ENTRY(BID_QUERY_PATH_NEX, ADX_NEX_PC);
        //酷派移动
        ADD_MODULE_ENTRY(BID_QUERY_PATH_KUPAI, ADX_KUPAI_MOBILE);
        //猎豹移动
        ADD_MODULE_ENTRY(BID_QUERY_PATH_LIEBAO, ADX_LIEBAO_MOBILE);
        // 360MAX
        ADD_MODULE_ENTRY(BID_QUERY_PATH_360MAX, ADX_360_MAX_PC);
        //一点资讯
        ADD_MODULE_ENTRY(BID_QUERY_PATH_YIDIAN, ADX_YIDIAN);
        // 2345
        ADD_MODULE_ENTRY(BID_QUERY_PATH_2345, ADX_2345_PC);

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
        case ADX_INMOBI:
            return new InmobiBiddingHandler();
        case ADX_NEX_PC:
            return new NexBiddingHandler();
        case ADX_KUPAI_MOBILE:
            return new KupaiBiddingHandler();
        case ADX_LIEBAO_MOBILE:
            return new LieBaoBiddingHandler();
        case ADX_360_MAX_PC:
            return new JuxiaoMaxBiddingHandler();
        case ADX_YIDIAN:
            return new YidianBidingHandler();
        case ADX_2345_PC:
            return new Adx2345BiddingHandler();
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

            requestCounter.increaseBidFailed(log.adInfo.adxid);
        } else {
            TaskThreadLocal * localData = threadData;
            HandleBidQueryTask * task = this;
            adservice::utility::PerformanceWatcher pw("HandleBidRequest", 20);
            (void)pw;
            bool bidResult = biddingHandler->filter(
                [localData, &log, task, &pw](AbstractBiddingHandler * adapter,
                                             std::vector<adselectv2::AdSelectCondition> & conditions) -> bool {
                    int seqId = 0;
                    seqId = localData->seqId;
                    //地域定向接入
                    IpManager & ipManager = IpManager::getInstance();
                    ScenarioManagerPtr scenarioManager = ScenarioManager::getInstance();
                    bool result = false;
                    int conditionIdx = 0;
                    std::vector<int64_t> bannedSolutions;
                    for (auto & condition : conditions) {
                        condition.dGeo = ipManager.getAreaByIp(condition.ip.data());
                        condition.dHour = adSelectTimeCodeUtc();
                        condition.scenario = scenarioManager->getScenario(condition.ip.data());
                        MT::common::SelectResult resp;
                        bool bAccepted = false;
                        bool searchOK = false;
                        {
                            adservice::utility::PerformanceWatcher searchPW("adselectClient->search", 20, &pw);
                            (void)searchPW;
                            if (bannedSolutions.size() > 0) {
                                condition.bannedSolutions = bannedSolutions;
                            }
                            searchOK = adSelectClient->search(seqId, false, condition, resp);
                        }
                        if (!searchOK) {
                            condition.mttyPid = resp.adplace.pId;
                            log.adInfo.mid = resp.adplace.mId;
                            bAccepted = false;
                        } else {
                            adservice::utility::PerformanceWatcher buildResultPW("buildResult", 2, &pw);
                            (void)buildResultPW;
                            adapter->buildBidResult(condition, resp, conditionIdx);
                            log.adInfo.mid = resp.adplace.mId;
                            bannedSolutions.push_back(log.adInfo.sid);
                            bAccepted = true;
                            result = true;
                        }
                        if (adapter->fillLogItem(condition, log, bAccepted)) {
                            adservice::utility::PerformanceWatcher doLogPW("doLog", 2, &pw);
                            (void)doLogPW;
                            task->doLog(log);
                        }
                        conditionIdx++;
                    }
                    return result;
                });
            if (bidResult) {
                biddingHandler->match(resp);

                requestCounter.increaseBidSuccess(log.adInfo.adxid);
            } else {
                biddingHandler->reject(resp);

                requestCounter.increaseBidFailed(log.adInfo.adxid);
            }
            needLog = false;
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
