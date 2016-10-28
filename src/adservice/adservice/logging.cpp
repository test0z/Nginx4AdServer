#include "logging.h"
#include "utility/http.h"
#include <boost/algorithm/string.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/support/date_time.hpp>
#include <iostream>
#include <vector>

LoggingLevel AdServiceLog::globalLoggingLevel = LoggingLevel::INFO_;

static const char * logLevelStr[]
    = { "TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL", "TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL" };

namespace {

class AdServiceLogBackendWraper : public boost::log::sinks::text_file_backend {
public:
    template <typename... ArgsT>
    explicit AdServiceLogBackendWraper(ArgsT const &... args)
        : boost::log::sinks::text_file_backend(args...)
    {
    }
    void consume(boost::log::record_view const & rec,
                 boost::log::sinks::text_file_backend::string_type const & formatted_message)
    {
        if (inDebugSession) {
            std::cerr << "log in debug session" << std::endl;
            if (debugSession != nullptr) {
                adservice::utility::HttpResponse & httpResponse = *(adservice::utility::HttpResponse *)debugSession;
                httpResponse << "[" << boost::log::extract<LoggingLevel>("Severity", rec) << "] " << formatted_message
                             << "\n";
            } else {
                std::cerr << "debugSession == nullptr" << std::endl;
            }
        }
    }
};
}

static std::vector<boost::shared_ptr<boost::log::sinks::asynchronous_sink<boost::log::sinks::text_file_backend>>>
    sinks_;
static boost::shared_ptr<boost::log::sinks::synchronous_sink<AdServiceLogBackendWraper>> debugSessionSink_;

template <typename CharT, typename TraitsT>
std::basic_ostream<CharT, TraitsT> & operator<<(std::basic_ostream<CharT, TraitsT> & strm, LoggingLevel lvl)
{
    if (static_cast<std::size_t>(lvl) < LoggingLevel::SIZE_) {
        strm << logLevelStr[lvl];
    } else {
        strm << static_cast<int>(lvl);
    }
    return strm;
}

adservice::utility::HttpResponse & operator<<(adservice::utility::HttpResponse & resp, LoggingLevel lvl)
{
    if (static_cast<std::size_t>(lvl) < LoggingLevel::SIZE_) {
        resp << logLevelStr[lvl];
    } else {
        resp << static_cast<int>(lvl);
    }
    return resp;
}

AdServiceLog::AdServiceLog(LoggingLevel lv, const std::string & logDirectory)
{
    currentLevel = lv;
    std::string logDir = logDirectory;
    if (*(logDir.end() - 1) != '/') {
        logDir += "/";
    }
    boost::log::sinks::file::rotation_at_time_point rotationPolicy(0, 0, 0);
    boost::log::sinks::asynchronous_sink<boost::log::sinks::text_file_backend>::formatter_type formatter
        = boost::log::expressions::stream
          << boost::log::expressions::attr<unsigned int>("RecordID") << " "
          << boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "[%Y-%m-%d %H:%M:%S]")
          << " " << boost::log::expressions::format_named_scope("Scope", boost::log::keywords::format = "[%n %f:%l]")
          << " " << boost::log::expressions::smessage;
    boost::log::sinks::synchronous_sink<AdServiceLogBackendWraper>::formatter_type syncFormatter = formatter;
    for (int i = 0; i < LoggingLevel::NORMAL_LOG_END_; i++) {
        std::string filenamePattern = std::string("/") + logLevelStr[i] + "_%Y_%m_%d.log";
        auto backend = boost::make_shared<boost::log::sinks::text_file_backend>(
            boost::log::keywords::file_name = logDir + filenamePattern,
            boost::log::keywords::time_based_rotation = rotationPolicy);
        backend->set_open_mode(std::ios_base::app);
        backend->auto_flush(true);

        auto sink
            = boost::make_shared<boost::log::sinks::asynchronous_sink<boost::log::sinks::text_file_backend>>(backend);
        sink->set_formatter(formatter);
        sink->set_filter(boost::log::expressions::attr<LoggingLevel>("Severity") == i);
        sinks_.emplace_back(sink);
    }
    for (int i = currentLevel; i < LoggingLevel::NORMAL_LOG_END_; ++i) {
        boost::log::core::get()->add_sink(sinks_[i]);
    }
    // DebugSession Sink
    auto debugBackend
        = boost::make_shared<AdServiceLogBackendWraper>(boost::log::keywords::file_name = logDir + "/DEBUG_SESSION.log",
                                                        boost::log::keywords::time_based_rotation = rotationPolicy);
    debugBackend->set_open_mode(std::ios_base::app);
    debugBackend->auto_flush(true);

    debugSessionSink_
        = boost::make_shared<boost::log::sinks::synchronous_sink<AdServiceLogBackendWraper>>(debugBackend);
    debugSessionSink_->set_formatter(syncFormatter);
    debugSessionSink_->set_filter(boost::log::expressions::attr<LoggingLevel>("Severity")
                                  >= LoggingLevel::DEBUG_SESSION_TRACE_);
    boost::log::core::get()->add_sink(debugSessionSink_);
    boost::log::core::get()->add_global_attribute("Scope", boost::log::attributes::named_scope());
    boost::log::core::get()->add_global_attribute("RecordID", boost::log::attributes::counter<unsigned int>(1));
    boost::log::core::get()->add_global_attribute("TimeStamp", boost::log::attributes::local_clock());
}

AdServiceLog::~AdServiceLog()
{
    for (int i = currentLevel; i < LoggingLevel::NORMAL_LOG_END_; ++i) {
        boost::log::core::get()->remove_sink(sinks_[i]);
        sinks_[i]->stop();
        sinks_[i]->flush();
        sinks_[i].reset();
    }
    debugSessionSink_->flush();
    debugSessionSink_.reset();
}
