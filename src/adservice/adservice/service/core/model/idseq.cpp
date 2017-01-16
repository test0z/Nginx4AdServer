#include "idseq.h"

namespace adservice {
namespace core {
    namespace model {

        UserIDEntity::UserIDEntity(int64_t t)
            : time_(t)
        {
        }

        int16_t UserIDEntity::id() const
        {
            return id_;
        }

        int64_t UserIDEntity::time() const
        {
            return time_;
        }

        void UserIDEntity::setId(int16_t id)
        {
            id_ = id > 0 ? id : -id;
        }

        void UserIDEntity::setTime(int64_t t)
        {
            time_ = t;
        }

        void UserIDEntity::record(const as_record * record)
        {
            id_ = as_record_get_int64(record, "id", 0);
        }
    }
}
}
