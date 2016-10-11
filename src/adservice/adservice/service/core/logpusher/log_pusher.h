//
// Created by guoze.lin on 16/2/14.
//

#ifndef ADCORE_LOGPUSHER_H
#define ADCORE_LOGPUSHER_H

#include "common/constants.h"
#include "common/spinlock.h"
#include "common/types.h"
#include "core/core_executor.h"
#include "utility/utility.h"
//#include "aliyun_log_producer.h"
#include "kafka_log_producer.h"
#include "logging.h"
#include <tbb/concurrent_hash_map.h>

namespace adservice {
namespace log {

	using namespace adservice::server;
	using namespace adservice::types;

	static constexpr int LOG_QUEUE_SIZE = 1024 * 1024;
	static const int LOGGER_THREAD_NUM = 100;

	class LogPusher;
	typedef std::shared_ptr<LogPusher> LogPusherPtr;

	typedef tbb::concurrent_hash_map<std::string, LogPusherPtr> LogPusherMap;
	typedef LogPusherMap::accessor LogPusherMapAccessor;

	class LogPusher {
	public:
		static LogPusherPtr getLogger(const std::string & name, const std::string & logConfigKey = CONFIG_LOG,
									  int ifnodefineThreads = 10, bool logLocal = false);

		static void removeLogger(const std::string & name);

	public:
		LogPusher(const char * logger = "log_default", int loggerThreads = LOGGER_THREAD_NUM, bool modeLocal = false,
				  const std::string & logConfigKey = CONFIG_LOG)
			: loggerName(logger)
			, executor(logger, false, loggerThreads, LOG_QUEUE_SIZE)
			, modeLocal(modeLocal)
			, loggerConfigKey(logConfigKey)
		{
			if (!modeLocal)
				initProducer();
			else
				producer = NULL;
		}
		~LogPusher()
		{
			if (producer != NULL)
				delete producer;
			LOG_INFO << "logger " << this->loggerName << " gone";
		}

		void initProducer()
		{
			producer = LogProducerFactory::createProducer(LogProducerType::LOG_KAFKA, loggerName, loggerConfigKey);
		}

		void reinitProducer()
		{
			if (producer != NULL) {
				delete producer;
            }
			initProducer();
		}

		void start()
		{
			if (!modeLocal && producer != NULL) {
				producer->start();
			}
			executor.start();
		}
		void stop()
		{
			executor.stop();
		}
		bool getWorkMode()
		{
			return modeLocal;
		}
		void setWorkMode(bool workLocal)
		{
			if (!workLocal && modeLocal == false && producer == NULL) {
                initProducer();
            }
			modeLocal = workLocal;
		}
		void startRemoteMonitor(const Message & msg);

		void push(std::shared_ptr<adservice::types::string> & logstring);
		void push(std::shared_ptr<adservice::types::string> && logstring);

		static void setLocalLogFilePrefix(const std::string & prefix)
		{
			localLogFilePrefix = prefix;
		}

		static const std::string & getLocalLogFilePrefix()
		{
			return localLogFilePrefix;
		}

	private:
	public:
		static struct spinlock lock;

	private:
		static LogPusherMap logMap;
		static std::string localLogFilePrefix;

	private:
		std::string loggerName;
		adservice::server::Executor executor;
		/// 工作模式,本地文件日志 或 远程日志
		bool modeLocal;
		std::string loggerConfigKey;
		LogProducer * producer;
	};
}
}

#endif // ADCORE_LOGPUSHER_H
