#include "core_cm_manager.h"
#include "config_types.h"
#include "logging.h"
#include <mtty/aerospike.h>

extern GlobalConfig globalConfig;
extern MT::common::Aerospike aerospikeClient;

namespace adservice {
namespace server {

    CookieMappingManager CookieMappingManager::instance_;

    adservice::core::model::MtUserMapping CookieMappingManager::getUserMapping(const std::string & userId)
    {
        MT::common::ASKey key(globalConfig.aerospikeConfig.nameSpace.c_str(), "CookieMapping", userId.c_str());
        adservice::core::model::MtUserMapping mapping;
        try {
            aerospikeClient.get(key, mapping);
        } catch (MT::common::AerospikeExcption & e) {
            LOG_ERROR << "获取 cookie mapping 失败，" << e.what() << ",code:" << e.error().code
                      << ",msg:" << e.error().message << ",调用堆栈：" << std::endl
                      << e.trace();
        }
        return mapping;
    }

    adservice::core::model::MtUserMapping CookieMappingManager::getUserMappingByKey(const std::string & key,
                                                                                    const std::string & value)
    {
        MT::common::ASQuery asQuery(globalConfig.aerospikeConfig.nameSpace.c_str(), "CookieMapping", 1);
        asQuery.whereEqStr(key, value);
        adservice::core::model::MtUserMapping mapping;
        try {
            aerospikeClient.queryWhere(asQuery, &mapping);
        } catch (MT::common::AerospikeExcption & e) {
            LOG_ERROR << " 查询cookie mapping 失败，" << e.what() << ",code: " << e.error().code
                      << ",msg:" << e.error().message << ",调用堆栈：" << std::endl
                      << e.trace();
        }
        return mapping;
    }

    bool CookieMappingManager::updateUserMapping(core::model::MtUserMapping & mapping)
    {
        MT::common::ASKey key(globalConfig.aerospikeConfig.nameSpace.c_str(), "CookieMapping", mapping.userId.c_str());
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

    bool CookieMappingManager::updateMappingAdxUid(const std::string & userId, int64_t adxId, const std::string & value)
    {
        core::model::MtUserMapping mapping;
        mapping.addMapping(adxId, value);
        MT::common::ASKey key(globalConfig.aerospikeConfig.nameSpace.c_str(), "CookieMapping", userId.c_str());
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

    bool CookieMappingManager::updateUserMappingAsync(adservice::core::model::MtUserMapping & mapping)
    {
        MT::common::ASKey key(globalConfig.aerospikeConfig.nameSpace.c_str(), "CookieMapping", mapping.userId.c_str());
        try {
            aerospikeClient.putAsync(key, mapping);
        } catch (MT::common::AerospikeExcption & e) {
            LOG_ERROR << "异步记录cookie mapping失败，" << e.what() << "，code：" << e.error().code << "，msg："
                      << e.error().message << "，调用堆栈：" << std::endl
                      << e.trace();
            return false;
        }
        return true;
    }

    bool
    CookieMappingManager::updateMappingAdxUidAsync(const std::string & userId, int64_t adxId, const std::string & value)
    {
        core::model::MtUserMapping mapping;
        mapping.addMapping(adxId, value);
        MT::common::ASKey key(globalConfig.aerospikeConfig.nameSpace.c_str(), "CookieMapping", userId.c_str());
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
}
}
