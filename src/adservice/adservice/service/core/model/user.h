#ifndef USER_H
#define USER_H

#include "common/spinlock.h"
#include <mtty/aerospike.h>
#include <mtty/constants.h>
#include <string>
#include <unordered_map>

namespace adservice {
namespace core {
    namespace model {
        bool QueryWhereCallback(const as_val * val, void * udata);

        class MtUserMapping : public MT::common::ASEntity {
            friend bool QueryWhereCallback(const as_val * val, void * udata);

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

            void reset();

            std::string cypherUid();

        protected:
            virtual void record(const as_record * record) override;
            virtual as_record * record() override;

        public:
            struct spinlock slock;
            std::string userId;
            std::string outerUserKey;
            std::string outerUserId;       // device id或者是平台uid
            std::string outerUserOriginId; //未加密的device id或者平台id
            std::string cypherUserId;
            bool needDeviceOriginId{ false };
            int64_t ttl{ DAY_SECOND * 30 };
        };
    }
}
}

#endif // USER_H
