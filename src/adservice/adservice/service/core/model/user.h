#ifndef USER_H
#define USER_H

#include "common/spinlock.h"
#include <mtty/aerospike.h>
#include <string>
#include <unordered_map>

namespace adservice {
namespace core {
    namespace model {

        class MtUserMapping : public MT::common::ASEntity {
        public:
            MtUserMapping()
            {
                slock.lock = 0;
            }

            static std::string adxUidKey(int64_t adxId);

            static const std::string & idfaKey();

            static const std::string & imeiKey();

            static const std::string & androidIdKey();

            static const std::string & macKey();

            static const std::string & userIdKey();

            bool isValid()
            {
                return !userId.empty();
            }

            void addMapping(int64_t adxId, const std::string & adxUid);

            void addDeviceMapping(const std::string & deviceKey, const std::string & deviceId);

            bool hasAdxUid(int64_t adxId) const;

            std::string getAdxUid(int adxId) const;

            std::string getDeviceId(const std::string & id) const;

            std::string getIdfa() const;

            std::string getIMei() const;

            void reset();

            std::string cypherUid();

        protected:
            void record(const as_record * record);
            as_record * record();

        public:
            struct spinlock slock;
            std::string userId;
            std::string cypherUserId;
            std::unordered_map<int64_t, std::string> adxUids;
            std::unordered_map<std::string, std::string> deviceIds;
        };
    }
}
}

#endif // USER_H
