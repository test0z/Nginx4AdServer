//
// Created by guoze.lin on 16/2/24.
//

#ifndef ADCORE_TIME_H
#define ADCORE_TIME_H

#include "common/types.h"
#include <chrono>
#include <ctime>
#include <mtty/constants.h>
#include <stddef.h>

namespace adservice {
namespace utility {

    namespace time {

        using namespace std::chrono;

        static const int64_t MTTY_SERVICE_TIME_BEGIN = 1443669071L; // mttyTimeBegin();
        static const int32_t timeZone = 8;
        static constexpr int32_t UTC_TIME_DIFF_SEC = timeZone * 3600;
        static constexpr int64_t UTC_TIME_DIFF_MS = UTC_TIME_DIFF_SEC * 1000;

        inline int64_t getMttyTimeBegin()
        {
            return MTTY_SERVICE_TIME_BEGIN;
        }

        inline int64_t getCurrentTimeStamp()
        {
            time_t currentTime;
            ::time(&currentTime);
            return (int64_t)currentTime;
        }

        inline int64_t getCurrentTimeStampMs()
        {
            milliseconds ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
            return ms.count();
        }

        std::string getCurrentTimeString();

        std::string getCurrentTimeGmtString();

        std::string gmtTimeStringFromTimeStamp(int64_t timestamp);

        std::string timeStringFromTimeStamp(int64_t timestamp);

        inline int32_t getCurrentTimeSinceMtty()
        {
            long currentTime = getCurrentTimeStamp();
            return (int32_t)(currentTime - MTTY_SERVICE_TIME_BEGIN);
        }

        /**
         * mtty时间到本地Unix时间戳
         */
        inline int64_t mttyTimeToUnixTimeStamp(int32_t mttyTime)
        {
            return mttyTime + MTTY_SERVICE_TIME_BEGIN;
        }

        /**
         * 本地时间戳到UTC时间戳
         */
        inline int64_t localTimeStamptoUtc(int64_t unixTimeStamp)
        {
            return unixTimeStamp;
        }

        inline int64_t getCurrentTimeStampUtc()
        {
            return localTimeStamptoUtc(getCurrentTimeStamp());
        }

        inline std::string getCurrentTimeUtcString()
        {
            return timeStringFromTimeStamp(getCurrentTimeStampUtc());
        }

        std::string adTimeSelectCode(int64_t timestamp);

        inline std::string adSelectTimeCodeUtc()
        {
            return adTimeSelectCode(getCurrentTimeStampUtc());
        }

        double getCurrentHourPercent();

        /**
         * 用于离线计算mtty基准时间
         */
        int64_t mttyTimeBegin();

        /**
         * 获取当前时间偏离当日零点的偏移秒数
         */
        int32_t getTimeSecondOfToday();

        /**
         * 获取当日零点Unix时间戳
         */
        int64_t getTodayStartTime();

        /**
         * 获取当日结束的Unix时间戳
         */
        inline int64_t getTodayEndTime()
        {
            return getTodayStartTime() + DAY_SECOND;
        }
    }

    class PerformanceWatcher {
    public:
        PerformanceWatcher(const std::string & n)
            : name(n)
        {
            beginTimeMs = time::getCurrentTimeStampMs();
        }
        ~PerformanceWatcher();

    private:
        int64_t beginTimeMs;
        std::string name;
    };
}
}

#endif // ADCORE_TIME_H
