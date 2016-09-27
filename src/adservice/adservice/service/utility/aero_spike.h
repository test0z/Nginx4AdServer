#ifndef __UTILITY_AERO_SPIKE_H__
#define __UTILITY_AERO_SPIKE_H__

#include <mutex>
#include <atomic>
#include <string>

#include <aerospike/aerospike.h>
#include <aerospike/as_record.h>
#include <aerospike/as_operations.h>
#include <aerospike/aerospike_key.h>
#include <muduo/base/Logging.h>

using namespace muduo;

namespace adservice {
namespace utility {

class AeroSpikeExcption : public std::exception {
public:
    AeroSpikeExcption(const std::string & what, const as_error & error);

    const as_error & error() const;

    const char * what() const noexcept override;

private:
    as_error error_;
    std::string what_;
};

class AeroSpike {
public:
    static AeroSpike instance;

    ~AeroSpike();

    bool connect();

    void close();

    aerospike * connection();

    operator bool();

    const as_error & error() const;

    const std::string & nameSpace() const;

    template<typename F, typename ... Args>
    static as_status noTimeOutExec(F && f, Args && ... args)
    {

        as_status result = AEROSPIKE_ERR_TIMEOUT;
        while (result == AEROSPIKE_ERR_TIMEOUT) {
            result = f(args ...);
        }

        return result;
    }

    template<typename RecordParser>
    bool get(as_key& key,RecordParser& obj){
        if(!instance&&!instance.connect()){
            LOG_ERROR<<"failed to connect to aerospike";
            return false;
        }
        as_error err;
        as_record* record = nullptr;
        if(aerospike_key_get(&connection_,&err,nullptr,&key,&record)!=AEROSPIKE_OK){
            LOG_ERROR << "aerospike get error, code:" << err.code << ", msg:" << err.message;
            as_key_destroy(&key);
            as_record_destroy(record);
            return false;
        }
        obj.parse(record);
        as_record_destroy(record);
        return true;
    }

    template<typename RecordParser>
    inline bool get(const char* ns,const char* setName,int64_t id,RecordParser& obj){
        as_key key;
        as_key_init_int64(&key,ns,setName,id);
        return get<RecordParser>(key,obj);
    }

private:
    as_config config_;
    aerospike connection_;
    as_error error_;
    std::string namespace_;

    std::mutex connectMutex_;
    std::mutex closeMutex_;

    std::atomic_bool connected_{ false };

    AeroSpike() = default;
};

}	// namespace utility
}	// namespace adservice

#endif	// __UTILITY_AERO_SPIKE_H__
