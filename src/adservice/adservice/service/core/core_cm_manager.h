#ifndef CORE_CM_MANAGER_H
#define CORE_CM_MANAGER_H

#include "model/idseq.h"
#include "model/user.h"
#include <memory>

namespace adservice {
namespace server {

    class CookieMappingQueryKeyValue {
    public:
        CookieMappingQueryKeyValue() = default;
        CookieMappingQueryKeyValue(const std::string & k, const std::string & v, bool adxCookie = true)
            : key(k)
            , value(v)
            , isAdxCookie(adxCookie)
        {
        }

        CookieMappingQueryKeyValue & rebind(const std::string & k, const std::string & v, bool adxCookie = true)
        {
            key = k;
            value = v;
            isAdxCookie = adxCookie;
            return *this;
        }

        void clearDeviceMapping()
        {
            key = "";
            value = "";
            deviceMappings.clear();
        }

        CookieMappingQueryKeyValue & rebindDevice(const std::string & k, const std::string & v, const std::string & v2)
        {
            if (!k.empty() && !v.empty()) {
                if (key.empty() || value.empty()) {
                    key = k;
                    value = v;
                }
                deviceMappings.insert({ k, { v, v2 } });
            }
            isAdxCookie = false;
            return *this;
        }

        bool isNull() const
        {
            return key.empty() || value.empty();
        }

        bool isAdxCookieKey() const
        {
            return isAdxCookie;
        }

    public:
        std::string key;
        std::string value;
        std::unordered_map<std::string, std::pair<std::string, std::string>> deviceMappings;
        bool isAdxCookie;
    };

    class CookieMappingManager {
    public:
        static CookieMappingManager & getInstance()
        {
            return instance_;
        }

    public:
        adservice::core::model::MtUserMapping
        getUserMappingByKey(const std::string & key, const std::string & value, bool isDevice);

        bool updateMappingAdxUid(const std::string & userId, int64_t adxId, const std::string & value);

        bool updateUserMappingAsync(adservice::core::model::MtUserMapping & mapping);

        bool updateMappingAdxUidAsync(const std::string & userId, int64_t adxId, const std::string & value);

        bool updateMappingDeviceAsync(const std::string & userId,
                                      const std::string & deviceIdType,
                                      const std::string & value,
                                      const std::string & originDeviceValue);

        void touchMapping(const std::string & key, const std::string & value, const std::string & userId);

        adservice::core::model::UserIDEntity newIdSeq();

        static adservice::core::model::UserIDEntity IdSeq()
        {
            return instance_.newIdSeq();
        }

    private:
        static CookieMappingManager instance_;
    };
}
}

#endif // CORE_CM_MANAGER_H
