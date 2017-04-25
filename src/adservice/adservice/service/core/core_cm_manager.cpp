#include "core_cm_manager.h"
#include "config_types.h"
#include "logging.h"
#include "utility/utility.h"
#include <chrono>
#include <mtty/aerospike.h>
#include <mtty/constants.h>

extern GlobalConfig globalConfig;
extern MT::common::Aerospike aerospikeClient;

namespace adservice {
namespace server {

    CookieMappingManager CookieMappingManager::instance_;

    //改成根据key查询
    adservice::core::model::MtUserMapping CookieMappingManager::getUserMappingByKey(const std::string & k,
                                                                                    const std::string & v,
                                                                                    bool isDevice,
                                                                                    bool & isTimeout)
    {
        adservice::core::model::MtUserMapping mapping;
        mapping.outerUserKey = k;
        if (isDevice) {
            mapping.needDeviceOriginId = true;
        }
        isTimeout = false;
        try {
            MT::common::ASKey key(globalConfig.aerospikeConfig.funcNamespace(AS_NAMESPACE_COOKIEMAPPING), k, v.c_str());
            isTimeout
                = (false == aerospikeClient.getWithTimeout(key, mapping, globalConfig.aerospikeConfig.getTimeoutMS));
        } catch (MT::common::AerospikeExcption & e) {
            //            LOG_DEBUG << " 查询cookie mapping 失败，" << e.what() << ",code: " << e.error().code
            //                      << ",msg:" << e.error().message << ",调用堆栈：" << std::endl
            //                      << e.trace();
        }
        return mapping;
    }

    // cm_reconstruct
    bool CookieMappingManager::updateMappingAdxUid(const std::string & userId,
                                                   int64_t adxId,
                                                   const std::string & value,
                                                   int64_t ttl)
    {
        core::model::MtUserMapping mapping;
        mapping.userId = userId;
        std::string k = core::model::MtUserMapping::adxUidKey(adxId);
        mapping.outerUserKey = k;
        mapping.outerUserId = value;
        mapping.ttl = ttl;
        MT::common::ASKey key(globalConfig.aerospikeConfig.funcNamespace(AS_NAMESPACE_COOKIEMAPPING), k, value);
        try {
            aerospikeClient.put(key, mapping);
        } catch (MT::common::AerospikeExcption & e) {
            LOG_ERROR << "记录cookie mapping失败，" << e.what() << "，code：" << e.error().code << "，msg："
                      << e.error().message << "，调用堆栈：" << std::endl
                      << e.trace();
            return false;
        }
        return true;
    }

    bool CookieMappingManager::updateMappingAdxUidAsync(const std::string & userId,
                                                        int64_t adxId,
                                                        const std::string & value,
                                                        int64_t ttl)
    {
        core::model::MtUserMapping mapping;
        mapping.userId = userId;
        std::string k = core::model::MtUserMapping::adxUidKey(adxId);
        mapping.outerUserKey = k;
        mapping.outerUserId = value;
        mapping.ttl = ttl;
        MT::common::ASKey key(globalConfig.aerospikeConfig.funcNamespace(AS_NAMESPACE_COOKIEMAPPING), k, value);
        try {
            aerospikeClient.putAsync(key, mapping);
        } catch (MT::common::AerospikeExcption & e) {
            LOG_ERROR << "记录cookie mapping失败，" << e.what() << "，code：" << e.error().code << "，msg："
                      << e.error().message << "，调用堆栈：" << std::endl
                      << e.trace();
            return false;
        }
        return true;
    }

    bool CookieMappingManager::updateMappingDeviceAsync(const std::string & userId,
                                                        const std::string & deviceIdType,
                                                        const std::string & value,
                                                        const std::string & originDeviceValue,
                                                        int64_t ttl)
    {
        core::model::MtUserMapping mapping;
        mapping.userId = userId;
        std::string k = deviceIdType;
        mapping.outerUserKey = k;
        mapping.outerUserId = value;
        mapping.ttl = ttl;
        if (!originDeviceValue.empty()) {
            mapping.needDeviceOriginId = true;
            mapping.outerUserOriginId = originDeviceValue;
        }
        MT::common::ASKey key(globalConfig.aerospikeConfig.funcNamespace(AS_NAMESPACE_COOKIEMAPPING), k, value);
        try {
            aerospikeClient.putAsync(key, mapping);
        } catch (MT::common::AerospikeExcption & e) {
            LOG_ERROR << "记录cookie mapping失败，" << e.what() << "，code：" << e.error().code << "，msg："
                      << e.error().message << "，调用堆栈：" << std::endl
                      << e.trace();
            return false;
        }
        return true;
    }

    adservice::core::model::UserIDEntity CookieMappingManager::newIdSeq()
    {
        int64_t time
            = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                  .count();
        adservice::core::model::UserIDEntity idEntity(time);
        try {
            MT::common::ASKey key(globalConfig.aerospikeConfig.funcNamespace(AS_NAMESPACE_IDSEQ), "id-counter", time);
            MT::common::ASOperation op(2, 15);
            op.addRead("id");
            op.addIncr("id", (int64_t)1ll);
            aerospikeClient.operate(key, op, idEntity);
        } catch (MT::common::AerospikeExcption & e) {
            idEntity.setId(int16_t(adservice::utility::rng::randomInt() & 0X0000FFFF));
        }
        return idEntity;
    }

    struct TouchFallbackData {
        std::string key;
        std::string value;
        std::string userId;
        TouchFallbackData(const std::string & k, const std::string & v, const std::string & u)
            : key(k)
            , value(v)
            , userId(u)
        {
        }
    };

    void touchMappingAsyncCallback(as_error * err, as_record * record, void * udata, as_event_loop * event_loop)
    {
        if (err != nullptr && err->code == 2 && udata != nullptr) { // if the mapping doesn't exist,update the mapping
            TouchFallbackData * touchData = (TouchFallbackData *)udata;
            CookieMappingManager & cmManager = CookieMappingManager::getInstance();
            cmManager.updateMappingDeviceAsync(touchData->userId, touchData->key, touchData->value, "");
        }
        if (udata != nullptr) {
            delete (TouchFallbackData *)udata;
        }
    }

    void CookieMappingManager::touchMapping(const std::string & k,
                                            const std::string & value,
                                            const std::string & userId,
                                            int64_t ttl)
    {
        MT::common::ASKey key(globalConfig.aerospikeConfig.funcNamespace(AS_NAMESPACE_COOKIEMAPPING), k, value);
        MT::common::ASOperation touchOperation(1, ttl);
        touchOperation.addTouch();
        try {
            TouchFallbackData * data = new TouchFallbackData(k, value, userId);
            aerospikeClient.operateAsync(key, touchOperation, touchMappingAsyncCallback, (void *)data);
        } catch (MT::common::AerospikeExcption & e) {
        } catch (std::bad_alloc & e) {
            LOG_ERROR << "touchMapping allocating memory failed";
        }
    }
}
}
