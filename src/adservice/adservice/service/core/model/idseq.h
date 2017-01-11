#ifndef IDSEQ_H
#define IDSEQ_H

#include <mtty/aerospike.h>

namespace adservice {
namespace core {
    namespace model {

        class UserIDEntity : public MT::common::ASEntity {
        public:
            explicit UserIDEntity(int64_t t);

            int16_t id() const;

            int64_t time() const;

            void setId(int16_t id);

            void setTime(int64_t t);

        protected:
            virtual void record(const as_record * record) override;

        private:
            int16_t id_{ 0 };
            int64_t time_{ 0 };
        };
    }
}
}

#endif // IDSEQ_H
