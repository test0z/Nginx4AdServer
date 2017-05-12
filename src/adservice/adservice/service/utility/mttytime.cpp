//
// Created by guoze.lin on 16/1/25.
//

#include "mttytime.h"
#include "logging.h"

namespace adservice {
namespace utility {
    namespace time {

        const int MTTY_START_YEAR = 2015;
        const int MTTY_START_MON = 10;
        const int MTTY_START_DAY = 1;
        const int MTTY_START_HOUR = 11;
        const int MTTY_START_MIN = 11;
        const int MTTY_START_SEC = 11;

        /**
         * 假定麦田服务开始的基准时间
         * 假定为2015-10-01 11:11:11开始
         */
        int64_t mttyTimeBegin()
        {
            struct tm beginTime;
            memset(&beginTime, sizeof(tm), 0);
            beginTime.tm_year = MTTY_START_YEAR - 1900;
            beginTime.tm_mon = MTTY_START_MON - 1;
            beginTime.tm_mday = MTTY_START_DAY;
            beginTime.tm_hour = MTTY_START_HOUR;
            beginTime.tm_min = MTTY_START_MIN;
            beginTime.tm_sec = MTTY_START_SEC;
            return (long)mktime(&beginTime);
        }

        int32_t getTimeSecondOfToday()
        {
            time_t currentTime;
            ::time(&currentTime);
            int64_t todayStartTime = getTodayStartTime();
            return currentTime - todayStartTime;
        }

        int64_t getTodayStartTime()
        {
            time_t currentTime;
            ::time(&currentTime);
            struct tm todayTime;
            memset(&todayTime, sizeof(tm), 0);
            localtime_r(&currentTime, &todayTime);
            todayTime.tm_hour = 0;
            todayTime.tm_min = 0;
            todayTime.tm_sec = 0;
            return (int64_t)mktime(&todayTime);
        }

        int64_t getCurrentHourTime()
        {
            time_t currentTime;
            ::time(&currentTime);
            struct tm todayTime;
            memset(&todayTime, sizeof(tm), 0);
            localtime_r(&currentTime, &todayTime);
            todayTime.tm_min = 0;
            todayTime.tm_sec = 0;
            return (int64_t)mktime(&todayTime);
        }

        double getCurrentHourPercent()
        {
            time_t currentTime;
            ::time(&currentTime);
            struct tm t;
            memset(&t, sizeof(tm), 0);
            localtime_r(&currentTime, &t);
            return (t.tm_min * 60 + t.tm_sec) * 1.0 / 3600;
        }

        std::string getCurrentTimeString()
        {
            time_t currentTime;
            ::time(&currentTime);
            struct tm t;
            memset(&t, sizeof(tm), 0);
            localtime_r(&currentTime, &t);
            char tstr[50];
            int len = sprintf(tstr, "%04d%02d%02d %02d:%02d:%02d", 1900 + t.tm_year, t.tm_mon + 1, t.tm_mday, t.tm_hour,
                              t.tm_min, t.tm_sec);
            return std::string(tstr, tstr + len);
        }

        std::string timeStringFromTimeStamp(int64_t timestamp)
        {
            struct tm t;
            memset(&t, sizeof(tm), 0);
            localtime_r((const time_t *)&timestamp, &t);
            char tstr[50];
            int len = sprintf(tstr, "%04d%02d%02d %02d:%02d:%02d", 1900 + t.tm_year, t.tm_mon + 1, t.tm_mday, t.tm_hour,
                              t.tm_min, t.tm_sec);
            return std::string(tstr, tstr + len);
        }

        std::string getCurrentTimeGmtString()
        {
            time_t currentTime;
            ::time(&currentTime);
            struct tm t;
            memset(&t, sizeof(tm), 0);
            gmtime_r(&currentTime, &t);
            char tstr[50];
            int len = sprintf(tstr, "%04d%02d%02d %02d:%02d:%02d", 1900 + t.tm_year, t.tm_mon + 1, t.tm_mday, t.tm_hour,
                              t.tm_min, t.tm_sec);
            return std::string(tstr, tstr + len);
        }

        std::string gmtTimeStringFromTimeStamp(int64_t timestamp)
        {
            struct tm t;
            memset(&t, sizeof(tm), 0);
            gmtime_r((const time_t *)&timestamp, &t);
            char tstr[50];
            int len = sprintf(tstr, "%04d%02d%02d %02d:%02d:%02d", 1900 + t.tm_year, t.tm_mon + 1, t.tm_mday, t.tm_hour,
                              t.tm_min, t.tm_sec);
            return std::string(tstr, tstr + len);
        }

        std::string adTimeSelectCode(int64_t timestamp)
        {
            struct tm t;
            memset(&t, sizeof(tm), 0);
            gmtime_r((const time_t *)&timestamp, &t);
            char scode[10];
            int len = sprintf(scode, "%d%02d", t.tm_wday == 0 ? 7 : t.tm_wday, t.tm_hour);
            return std::string(scode, scode + len);
        }
    }

    PerformanceWatcher::~PerformanceWatcher()
    {
        int64_t spent = time::getCurrentTimeStampMs() - beginTimeMs;
        LOG_DEBUG << name << " time collapses:" << spent << "ms";
        if (spent >= threshhold) {
            LOG_WARN << name << " consumes too much time " << spent << "ms";
        }
    }
}
}
