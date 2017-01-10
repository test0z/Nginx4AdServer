#include "user.h"
#include <mtty/constants.h>
#include <mtty/mtuser.h>
#include <vector>

namespace adservice {
namespace core {
    namespace model {

        const std::string MAPPING_KEY_IDFA = "idfa";
        const std::string MAPPING_KEY_IMEI = "imei";
        const std::string MAPPING_KEY_MAC = "mac";
        const std::string MAPPING_KEY_ANDROIDID = "androidId";
        const std::string MAPPING_KEY_USER = "user_id";

        std::string MtUserMapping::adxUidKey(int64_t adxId)
        {
            std::string asBinName = "adxuid_";
            asBinName += std::to_string(adxId);
            return asBinName;
        }

        const std::string & MtUserMapping::idfaKey()
        {
            return MAPPING_KEY_IDFA;
        }

        const std::string & MtUserMapping::imeiKey()
        {
            return MAPPING_KEY_IMEI;
        }

        const std::string & MtUserMapping::androidIdKey()
        {
            return MAPPING_KEY_ANDROIDID;
        }

        const std::string & MtUserMapping::macKey()
        {
            return MAPPING_KEY_MAC;
        }

        const std::string & MtUserMapping::userIdKey()
        {
            return MAPPING_KEY_USER;
        }

        void MtUserMapping::reset()
        {
            userId = "";
            cypherUserId = "";
            if (!adxUids.empty()) {
                adxUids.clear();
            }
            if (!deviceIds.empty()) {
                deviceIds.clear();
            }
        }

        std::string MtUserMapping::cypherUid()
        {
            if (cypherUserId.empty()) {
                MT::User::UserID uId(userId, false);
                if (uId.isValid()) {
                    cypherUserId = uId.cipher();
                }
                return cypherUserId;
            } else {
                return cypherUserId;
            }
        }

        void MtUserMapping::addMapping(int64_t adxId, const std::string & adxUid)
        {
            adxUids[adxId] = adxUid;
        }

        void MtUserMapping::addDeviceMapping(const std::string & deviceKey, const std::string & deviceId)
        {
            deviceIds[deviceKey] = deviceId;
        }

        bool MtUserMapping::hasAdxUid(int64_t adxId) const
        {
            return adxUids.find(adxId) != adxUids.end();
        }

        std::string MtUserMapping::getAdxUid(int adxId) const
        {
            const auto & iter = adxUids.find(adxId);
            if (iter != adxUids.end()) {
                return iter->second;
            } else {
                return "";
            }
        }

        std::string MtUserMapping::getDeviceId(const std::string & id) const
        {
            const auto & iter = deviceIds.find(id);
            if (iter != deviceIds.end()) {
                return iter->second;
            } else {
                return "";
            }
        }

        std::string MtUserMapping::getIdfa() const
        {
            return getDeviceId(MAPPING_KEY_IDFA);
        }

        std::string MtUserMapping::getIMei() const
        {
            return getDeviceId(MAPPING_KEY_IMEI);
        }

        /**
         *insert into test.CookieMapping
         *(PK,adxuid_1,adxuid_15,adxuid_6,adxuid_14,adxuid_21,adxuid_8,adxuid_19,adxuid_13,adxuid_5,adxuid_20,adxuid_10,idfa,imei,user_id)
         *values ("12345678","a","b","c","d","e","f","g","h","i","j","k","l","m","n")
         * @brief MtUserMapping::record
         * @param record
         */
        void MtUserMapping::record(const as_record * record)
        {
            static std::vector<int64_t> adxIds{ ADX_TANX,         ADX_TANX_MOBILE,     ADX_BAIDU,
                                                ADX_GUANGYIN,     ADX_GUANGYIN_MOBILE, ADX_SOHU_PC,
                                                ADX_SOHU_MOBILE,  ADX_TENCENT_GDT,     ADX_YOUKU,
                                                ADX_YOUKU_MOBILE, ADX_NETEASE_MOBILE,  ADX_INMOBI };
            static std::vector<std::string> deviceKeys
                = { MAPPING_KEY_IDFA, MAPPING_KEY_IMEI, MAPPING_KEY_ANDROIDID, MAPPING_KEY_MAC };
            if (spinlock_trylock(&slock)) {
                this->reset();
                for (auto id : adxIds) {
                    this->adxUids.insert({ id, getStr(record, adxUidKey(id).c_str()) });
                }
                for (const auto & deviceKey : deviceKeys) {
                    this->deviceIds.insert({ deviceKey, getStr(record, deviceKey.c_str()) });
                }
                this->userId = getStr(record, MAPPING_KEY_USER.c_str());
            }
        }

        as_record * MtUserMapping::record()
        {
            int size = adxUids.size() + deviceIds.size() + 1;
            if (record_ == nullptr) {
                record_ = as_record_new(size);
            }
            for (auto & iter : adxUids) {
                int64_t adxId = iter.first;
                as_record_set_str(record_, adxUidKey(adxId).c_str(), iter.second.c_str());
            }
            for (auto & iter : deviceIds) {
                as_record_set_str(record_, iter.first.c_str(), iter.second.c_str());
            }
            if (!userId.empty()) {
                as_record_set_str(record_, MAPPING_KEY_USER.c_str(), userId.c_str());
            }
            return record_;
        }
    }
}
}