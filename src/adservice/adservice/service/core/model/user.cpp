#include "user.h"
#include <mtty/constants.h>
#include <vector>

namespace adservice {
namespace core {
    namespace model {

        const std::string MAPPING_KEY_IDFA = "idfa";
        const std::string MAPPING_KEY_IMEI = "imei";
        const std::string MAPPING_KEY_USER = "user_id";

        std::string MtUserMapping::adxUidKey(int64_t adxId)
        {
            std::string asBinName = "adxuid_";
            asBindName += std::to_string(adxId);
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

        void MtUserMapping::addMapping(int64_t adxId, const std::string & adxUid)
        {
            adxUids[adxId] = adxUid;
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

        void MtUserMapping::record(const as_record * record)
        {
            static std::vector<int64_t> adxIds{ ADX_TANX,         ADX_TANX_MOBILE,     ADX_BAIDU,
                                                ADX_GUANGYIN,     ADX_GUANGYIN_MOBILE, ADX_SOHU_PC,
                                                ADX_SOHU_MOBILE,  ADX_TENCENT_GDT,     ADX_YOUKU,
                                                ADX_YOUKU_MOBILE, ADX_NETEASE_MOBILE,  ADX_INMOBI };
            static std::vector<std::string> deviceKeys = { MAPPING_KEY_IDFA, MAPPING_KEY_IMEI };
            for (auto id : adxids) {
                this->adxUids.insert({ id, getStr(rec, adxUidKey(id).c_str()) });
            }
            for (const auto & deviceKey : deviceKeys) {
                this->deviceIds.insert({ deviceKey, getStr(rec, deviceKey.c_str()) });
            }
            this->userId = getStr(rec, MAPPING_KEY_USER);
        }

        as_record * MtUserMapping::record()
        {
            int size = adxUids.size() + deviceIds.size() + 1;
            if (record_ == nullptr) {
                record_ = as_record_new(size);
            }
            if (record_ != nullptr && record_->bins.size != size) {
                as_record_destroy(record_);
                record_ = as_record_new(size);
            }
            for (auto & iter : adxUids) {
                int64_t adxId = iter.first;
                as_record_set_str(record_, adxUidKey(adxId).c_str(), iter.second.c_str());
            }
            for (auto & iter : deviceIds) {
                as_record_set_str(record_, iter.first.c_str(), iter.second.c_str());
            }
            as_record_set_str(record_, MAPPING_KEY_USER.c_str(), userId.c_str());
            return record_;
        }
    }
}
}
