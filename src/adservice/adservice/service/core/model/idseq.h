#ifndef IDSEQ_H
#define IDSEQ_H

#include <mtty/aerospike.h>

namespace adservice {
namespace core {
    namespace model {

        class UserIDEntity : public MT::common::ASEntity {
        public:
            explicit UserIDEntity(int64_t t)
                : time_(t)
            {
            }

            int16_t id() const
            {
                return id_;
            }

            int64_t time() const
            {
                return time_;
            }

            void setId(int16_t id)
            {
                id_ = id > 0 ? id : -id;
            }

            void setTime(int64_t t)
            {
                time_ = t;
            }

        protected:
            virtual void record(const as_record * record) override
            {
                id_ = as_record_get_int64(record, "id", 0);
            }

        private:
            int16_t id_{ 0 };
            int64_t time_{ 0 };
        };
    }
}
}

#endif // IDSEQ_H
