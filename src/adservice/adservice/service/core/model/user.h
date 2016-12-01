#ifndef USER_H
#define USER_H

#include <map>
#include <mtty/aerospike.h>
#include <string>

namespace adservice {
namespace core {
    namespace model {

        class MtUserMapping : public MT::common::ASEntity {
        public:
            static std::string adxUidKey(int64_t adxId);

            static const std::string & idfaKey();

            static const std::string & imeiKey();

            bool isValid()
            {
                return false;
            }

            void addMapping(int64_t adxId, const std::string & adxUid);

            bool hasAdxUid(int64_t adxId) const;

            std::string getAdxUid(int adxId) const;

            std::string getDeviceId(const std::string & id) const;

            std::string getIdfa() const;

            std::string getIMei() const;

        protected:
            void record(const as_record * record);
            as_record * record();

        public:
            std::string userId;
            std::map<int64_t, std::string> adxUids;
            std::map<std::string, std::string> deviceIds;
        };
    }
}
}

#endif // USER_H
