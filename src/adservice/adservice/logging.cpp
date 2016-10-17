#include "logging.h"
#include <vector>
#include <boost/log/core.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/expressions.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/sinks/async_frontend.hpp>

LoggingLevel AdServiceLog::globalLoggingLevel = LoggingLevel::INFO_;

static const char * logLevelStr[] = { "TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL" };

static std::vector<boost::shared_ptr<boost::log::sinks::asynchronous_sink<boost::log::sinks::text_file_backend>>> sinks_;


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

AdServiceLog::AdServiceLog(LoggingLevel lv,const std::string& logDirectory)
{
	currentLevel = lv;
	std::string logDir = logDirectory;
	if (*(logDir.end() - 1) != '/') {
		logDir += "/";
	}
	boost::log::sinks::file::rotation_at_time_point rotationPolicy(0, 0, 0);
	boost::log::sinks::asynchronous_sink<boost::log::sinks::text_file_backend>::formatter_type formatter =
			boost::log::expressions::stream
			<< boost::log::expressions::attr<unsigned int>("RecordID") << " "
			<< boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "[%Y-%m-%d %H:%M:%S]") << " "
			<< boost::log::expressions::format_named_scope("Scope", boost::log::keywords::format = "[%n %f:%l]") << " "
			<< boost::log::expressions::smessage;
	for(int i=0;i<LoggingLevel::SIZE_;i++){
		std::string filenamePattern = std::string("/") + logLevelStr[i] + "_%Y_%m_%d.log";
		auto backend = boost::make_shared<boost::log::sinks::text_file_backend>(
				boost::log::keywords::file_name = logDir + filenamePattern,
				boost::log::keywords::time_based_rotation = rotationPolicy);
		backend->set_open_mode(std::ios_base::app);
		backend->auto_flush(true);

		auto sink = boost::make_shared<boost::log::sinks::asynchronous_sink<boost::log::sinks::text_file_backend>>(backend);
		sink->set_formatter(formatter);
		sink->set_filter(boost::log::expressions::attr<LoggingLevel>("Severity") == i);
		sinks_.emplace_back(sink);
	}
	for (int i = currentLevel; i < LoggingLevel::SIZE_; ++i) {
		boost::log::core::get()->add_sink(sinks_[i]);
	}
	boost::log::core::get()->add_global_attribute("Scope", boost::log::attributes::named_scope());
	boost::log::core::get()->add_global_attribute("RecordID", boost::log::attributes::counter<unsigned int>(1));
	boost::log::core::get()->add_global_attribute("TimeStamp", boost::log::attributes::local_clock());
}

AdServiceLog::~AdServiceLog()
{
	for (int i = currentLevel; i < LoggingLevel::SIZE_; ++i) {
		boost::log::core::get()->remove_sink(sinks_[i]);
		sinks_[i]->stop();
		sinks_[i]->flush();
		sinks_[i].reset();
	}
}
