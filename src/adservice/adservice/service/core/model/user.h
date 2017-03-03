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
                spinlock_init(&slock);
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
            virtual void record(const as_record * record) override;
            virtual as_record * record() override;

        public:
            struct spinlock slock;
            std::string userId;
            std::string outerUserKey;
            std::string outerUserId; // device id或者是平台uid
            std::string cypherUserId;
        };
    }
}
}

#endif // USER_H
