#include "core_cm_manager.h"
#include "config_types.h"
#include "logging.h"
#include "utility/utility.h"
#include <chrono>
#include <mtty/aerospike.h>

extern GlobalConfig globalConfig;
extern MT::common::Aerospike aerospikeClient;

namespace adservice {
namespace server {

    CookieMappingManager CookieMappingManager::instance_;

    adservice::core::model::MtUserMapping CookieMappingManager::getUserMapping(const std::string & userId)
    {
        MT::common::ASKey key(globalConfig.aerospikeConfig.nameSpace.c_str(), "CookieMapping", userId);
        adservice::core::model::MtUserMapping mapping;
        try {
            aerospikeClient.get(key, mapping);
        } catch (MT::common::AerospikeExcption & e) {
            LOG_DEBUG << "获取 cookie mapping 失败，" << e.what() << ",code:" << e.error().code
                      << ",msg:" << e.error().message << ",调用堆栈：" << std::endl
                      << e.trace();
        }
        return mapping;
    }

    adservice::core::model::MtUserMapping CookieMappingManager::getUserMappingByKey(const std::string & key,
                                                                                    const std::string & value)
    {
        adservice::core::model::MtUserMapping mapping;
        try {
            as_query query;
            as_query_init(&query, globalConfig.aerospikeConfig.nameSpace.c_str(), "CookieMapping");
            as_query_where_inita(&query, 1);
            as_query_where(&query, key.c_str(), as_string_equals(value.c_str()));
            aerospikeClient.queryWhere(query, &mapping);
        } catch (MT::common::AerospikeExcption & e) {
            LOG_ERROR << " 查询cookie mapping 失败，" << e.what() << ",code: " << e.error().code
                      << ",msg:" << e.error().message << ",调用堆栈：" << std::endl
                      << e.trace();
        }
        return mapping;
    }

    bool CookieMappingManager::updateUserMapping(core::model::MtUserMapping & mapping)
    {
        if (mapping.userId.empty()) {
            return false;
        }
        MT::common::ASKey key(globalConfig.aerospikeConfig.nameSpace.c_str(), "CookieMapping", mapping.userId);
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
        mapping.userId = userId;
        mapping.addMapping(adxId, value);
        MT::common::ASKey key(globalConfig.aerospikeConfig.nameSpace.c_str(), "CookieMapping", userId);
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
        if (mapping.userId.empty()) {
            return false;
        }
        MT::common::ASKey key(globalConfig.aerospikeConfig.nameSpace.c_str(), "CookieMapping", mapping.userId);
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
        mapping.userId = userId;
        mapping.addMapping(adxId, value);
        MT::common::ASKey key(globalConfig.aerospikeConfig.nameSpace.c_str(), "CookieMapping", userId);
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
            MT::common::ASKey key(globalConfig.aerospikeConfig.nameSpace.c_str(), "userid-counter", time);
            MT::common::ASOperation op(2, 60);
            op.addRead("id")(;
            op.addIncr("id", (int64_t)1ll);
            aerospikeClient.operate(key, op, idEntity);
        } catch (MT::common::AerospikeExcption & e) {
            idEntity.setId(int16_t(adservice::utility::rng::randomInt() & 0X0000FFFF));
        }
        return idEntity;
    }
}
}
