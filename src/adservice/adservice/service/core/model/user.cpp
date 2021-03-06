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
        const std::string MAPPING_OUTER_ORIGINID = "origin_id";

        bool QueryWhereCallback(const as_val * val, void * udata)
        {
            if (val == nullptr) {
                return true;
            }
            as_record * record = as_record_fromval(val);
            std::vector<std::shared_ptr<adservice::core::model::MtUserMapping>> * localVector
                = static_cast<std::vector<std::shared_ptr<adservice::core::model::MtUserMapping>> *>(udata);
            if (record != nullptr) {
                std::shared_ptr<adservice::core::model::MtUserMapping> entity
                    = std::make_shared<adservice::core::model::MtUserMapping>();
                entity->needDeviceOriginId = true;
                entity->record(record);
                localVector->push_back(entity);
            }
            return true;
        }

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
            outerUserId = "";
            outerUserKey = "";
            needDeviceOriginId = false;
            outerUserOriginId = "";
            ttl = DAY_SECOND * 30;
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

        /**
         *insert into test.CookieMapping
         *(PK,adxuid_1,adxuid_15,adxuid_6,adxuid_14,adxuid_21,adxuid_8,adxuid_19,adxuid_13,adxuid_5,adxuid_20,adxuid_10,idfa,imei,user_id)
         *values ("12345678","a","b","c","d","e","f","g","h","i","j","k","l","m","n")
         * @brief MtUserMapping::record
         * @param record
         */
        void MtUserMapping::record(const as_record * record)
        {
            this->userId = getStr(record, MAPPING_KEY_USER.c_str());
            this->outerUserId = getStr(record, outerUserKey.c_str());
            if (this->needDeviceOriginId) {
                this->outerUserOriginId = getStr(record, MAPPING_OUTER_ORIGINID.c_str());
            }
        }

        as_record * MtUserMapping::record()
        {
            int size = this->needDeviceOriginId ? 3 : 2;
            if (record_ == nullptr) {
                record_ = as_record_new(size);
            }
            record_->ttl = ttl;
            if (!userId.empty()) {
                as_record_set_str(record_, MAPPING_KEY_USER.c_str(), userId.c_str());
            }
            if (!outerUserId.empty()) {
                as_record_set_str(record_, outerUserKey.c_str(), outerUserId.c_str());
            }
            if (!outerUserOriginId.empty()) {
                as_record_set_str(record_, MAPPING_OUTER_ORIGINID.c_str(), outerUserOriginId.c_str());
            }
            return record_;
        }
    }
}
}
