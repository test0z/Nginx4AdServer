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

    /**
     * 先考虑几个问题：
     * 1）对于一个新用户，重来没有过麦田cookie,在服务端也没有任何的用户标识，rtb流量因为找不到服务端用户标识而触发cookiemapping,
     *   对于tanx，youku一类的网页平台，由于cookiemapping请求和曝光请求以异步请求形式同时嵌入到网页中，这时种cookie有可能引发并发问题：
     *   即m请求和s请求同时到达服务端,如果由于都没有旧cookie，于是都各自生成一个，真正被种植在客户端的cookie是不确定的，意味着其中一个请求的影响将被忽略。
     * 2）对于一个老用户,在麦田服务端存在用户标识，但是客户端标识被手动清除或者过期，如果客户端正好访问麦田的模块由于cookie不存在而重新种入新cookie,
     *   将使客户端标识和服务端标识发生断层。如果是全部cookie都被清除则可以忽略影响。
     * 3）移动平台rtb流量引发的cookiemapping,由于移动app的展示形式是没有cookie的,意味着依赖cookie所实现的逻辑将无法正常工作。
     * 4）平台不支持嵌入image标签式的cookiemapping机制，如ssp平台
     * 解决方案：竞价模块生成一个u参数，u参数为用户标识的唯一id对应的cookie,
     * 将这个参数同时加入各个模块的url中，并且在处理各个模块时优先读取这个参数作为用户标识（当发现u参数与目前浏览器cookie冲突时,校正cookie);
     * 不支持image标签的情况下，在需要url的地方同时嵌入u参数，而且异步调用cookiemapping模块的逻辑并将这个唯一标识传入到模块中。
     * 附加问题：
     * 5）对于一个老用户，假设在服务端存在一个麦田用户标识和各家平台cookie的映射,其中包含tanxcookie，有一天tanxcookie过期了，在tanx竞价流量中无法根据tanx
     *   的cookie反查麦田标识，从而引发cookiemapping,所有url被引入新的u参数。分两种情况讨论：
     *   （1）麦田cookie依然存在,此时如果优先使用u参数覆盖旧cookie将导致原来的映射失效；所以需要制定一个完善的u参数与cookie校正方案
     *   （2）麦田cookie也不复存在，此时会必定使用u参数作为新标识种cookie,新标识将在服务端产生新的记录；此时对应其它cookie未过期的平台的情况，意味着这个用户在
     *       服务端将同时存在多条标识，而且必然引起频繁的标识冲突。如果这种情况是由用户清除所有浏览器cookie引起的，那么影响可以忽略。否则这种问题是无法解决的，建议是
     *       尽量避免麦田cookie过期。
     * u参数与cookie校正方案：
     *    假定cookie与u参数都合法,但不相同。u参数的追溯时间与cookie的追溯时间作比较,cookie追溯时间更早，则使用cookie。
     *    假定cookie不合法或不存在，但u合法，使用u参数。
     *    假定cookie合法，但u不合法或不存在，使用cookie。
     *    假定cookie与u均不合法或不存在,生成新cookie。
     */

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
