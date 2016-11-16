#ifndef __CORE_MODEL_SOURCE_RECORD_H__
#define __CORE_MODEL_SOURCE_RECORD_H__

#include "utility/url.h"

#include <mtty/aerospike.h>

namespace adservice {
namespace core {
	namespace model {

		class SourceId : public MT::common::ASEntity {
		public:
			SourceId() = default;

			const std::string & get() const;

		private:
			std::string val_;

		protected:
			virtual void record(const as_record * record) override;
		};

		class SourceRecord : public MT::common::ASEntity {
		public:
			SourceRecord() = default;

			SourceRecord(utility::url::ParamMap & paramMap, protocol::log::LogItem & log);

			void record(const as_record & record);

			int64_t time() const;
			const std::string & advId() const;
			const std::string & sid() const;
			const std::string & adxId() const;
			const std::string & mtUid() const;
			const std::string & pid() const;
			const std::string & adxPid() const;
			const std::string & requestId() const;
			const std::string & createId() const;
			const std::string & geoId() const;
			const std::string & refererUrl() const;
			const std::string & bidPrice() const;

		private:
			int64_t time_{ 0 };      // 最后更新时间
			std::string advId_;      // 广告主ID
			std::string sid_;        // 推广单ID
			std::string adxId_;      // Adx平台ID
			std::string mtUid_;      // 麦田用户ID
			std::string pid_;        // 我方广告位ID
			std::string adxPid_;     // adx的广告位ID
			std::string requestId_;  // 请求ID
			std::string createId_;   // 创意ID
			std::string geoId_;      // 地域ID
			std::string refererUrl_; // 来源页
			std::string bidPrice_;   // 出价价格

		protected:
			virtual void record(const as_record * record) override;
		};

	} // namespace model
} // namespace core
} // namespace adservice

#endif // __CORE_MODEL_SOURCE_RECORD_H__
