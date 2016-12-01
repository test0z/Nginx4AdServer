#ifndef CORE_CM_MANAGER_H
#define CORE_CM_MANAGER_H

#include "model/user.h"
#include <memory>

namespace adservice {
namespace server {

    class CookieMappingQueryKeyValue {
    public:
        CookieMappingManager() = default;
        CookieMappingQueryKeyValue(const std::string & k, const std::string & v)
            : key(k)
            , value(v)
        {
        }
        bool isNull()
        {
            return key.empty() || value.empty();
        }

    public:
        std::string key;
        std::string value;
    };

    class CookieMappingManager {
    public:
        static CookieMappingManager & getInstance()
        {
            return instance_;
        }

        CookieMappingManager(const std::string & ns, const std::string & set);

    public:
        adservice::core::model::MtUserMapping getUserMapping(const std::string & userId);

        adservice::core::model::MtUserMapping getUserMappingByKey(const std::string & key, const std::string & value);

        bool updateUserMapping(const adservice::core::model::MtUserMapping & mapping);

        bool updateMappingAdxUid(const std::string & userId, int64_t adxId, const std::string & value);

        void updateUserMappingAsync(const adservice::core::model::MtUserMapping & mapping);

        void updateMappingAdxUidAsync(const std::string & userId, int64_t adxId, const std::string & value);

    private:
        static CookieMappingManager instance_;
    };
}
}

#endif // CORE_CM_MANAGER_H
