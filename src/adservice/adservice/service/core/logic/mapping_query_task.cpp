#include "mapping_query_task.h"
#include "core/core_cm_manager.h"
#include "core/model/user.h"
#include "utility/utility.h"
#include <mtty/mtuser.h>
#include <tuple>

namespace adservice {
namespace corelogic {

    using namespace adservice::server;
    using namespace adservice::core;

    namespace {
        std::string getAdxMappingKey(int64_t adxId)
        {
            switch (adxId) {
            case ADX_TANX:
            case ADX_TANX_MOBILE:
                return "tanx_tid";
            case ADX_YOUKU:
            case ADX_YOUKU_MOBILE:
                return "mzid";
            }
            return "";
        }
    }

    std::string HandleMappingQueryTask::imageData;

    protocol::log::LogPhaseType HandleMappingQueryTask::currentPhase()
    {
        return protocol::log::LogPhaseType::MAPPING;
    }

    int HandleMappingQueryTask::expectedReqStatus()
    {
        return 200;
    }

    void HandleMappingQueryTask::onError(std::exception & e, adservice::utility::HttpResponse & resp)
    {
        LOG_ERROR << "error occured in HandleMappingQueryTask:" << e.what();
    }

    void HandleMappingQueryTask::prepareImage()
    {
        static const std::string image = "R0lGODlhAQABAAAAACw=";
        urlsafe_base64decode(image, imageData);
    }

    /**
     * @brief HandleMappingQueryTask::customLogic
     * @param paramMap
     * @param log
     * @param resp
     */
    void HandleMappingQueryTask::customLogic(ParamMap & paramMap, protocol::log::LogItem & log,
                                             adservice::utility::HttpResponse & resp)
    {
        try {
            const std::string & uId = log.userId;
            // todo:获取url中和cookiemapping相关的参数,更新到这个uid对应的记录中，并且设置映射表条目的过期时间
            CookieMappingManager & cmManager = CookieMappingManager::getInstance();
            int64_t adxId = log.adInfo.adxid;
            std::string adxUidKey = getAdxMappingKey(adxId);
            std::string adxUid = paramMap.find(adxUidKey) != paramMap.end() ? paramMap[adxUidKey] : "";
            if (!adxUid.empty()) {
                char buf[1024];
                std::string decodeAdxUid;
                utility::url::urlDecode_f(adxUid, decodeAdxUid, buf);
                bool bSuccess = cmManager.updateMappingAdxUid(uId, adxId, decodeAdxUid);
                if (!bSuccess) {
                    LOG_WARN << "failed to update cookie mapping,uid:" << uId << ",adxId:" << adxId
                             << ",adxUid:" << adxUid;
                }
            }
            //生成1x1图片
            resp.status(302, "OK");
            resp.set_header("Location", "http://mtty-cdn.mtty.com/1x1.gif");
            resp.set_body("m");
            resp.set_header("Pragma", "no-cache");
            resp.set_header("Cache-Control", "no-cache,no-store;must-revalidate");
            resp.set_header("P3p",
                            "CP=\"CURa ADMa DEVa PSAo PSDo OUR BUS UNI PUR INT DEM STA PRE COM NAV OTC NOI DSP COR\"");
        } catch (std::exception & e) {
            log.reqStatus = 500;
            LOG_ERROR << "exception thrown during HandleMappingQueryTask::customLogic,e:" << e.what();
        }
    }
}
}
