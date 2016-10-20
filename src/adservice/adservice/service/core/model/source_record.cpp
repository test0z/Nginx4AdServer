#include "source_record.h"

#include "common/constants.h"

namespace adservice {
namespace core {
	namespace model {

		SourceRecord::SourceRecord(utility::url::ParamMap & paramMap, protocol::log::LogItem & log)
			: time_(log.timeStamp)
			, advId_(paramMap[URL_ADOWNER_ID])
			, sid_(paramMap[URL_EXEC_ID])
			, adxId_(paramMap[URL_ADX_ID])
			, mtUid_(log.userId)
			, pid_(paramMap[URL_MTTYADPLACE_ID])
			, adxPid_(paramMap[URL_ADPLACE_ID])
			, requestId_(paramMap[URL_EXPOSE_ID])
			, createId_(paramMap[URL_CREATIVE_ID])
			, geoId_(paramMap[URL_AREA_ID])
			, refererUrl_(paramMap[URL_REFERER])
			, bidPrice_(paramMap[URL_BID_PRICE])
		{
		}

		SourceRecord::~SourceRecord()
		{
			if (record_) {
				as_record_destroy(record_);
			}
		}

		void SourceRecord::record(const as_record & record)
		{
			time_ = as_record_get_int64(&record, "latest_time", -1);
			advId_ = as_record_get_str(&record, "adv_id");
			sid_ = as_record_get_str(&record, "sid");
			adxId_ = as_record_get_str(&record, "adx_id");
			mtUid_ = as_record_get_str(&record, "mt_uid");
			pid_ = as_record_get_str(&record, "pid");
			adxPid_ = as_record_get_str(&record, "adxpid");
			requestId_ = as_record_get_str(&record, "request_id");
			createId_ = as_record_get_str(&record, "create_id");
			geoId_ = as_record_get_str(&record, "geo_id");
			refererUrl_ = as_record_get_str(&record, "referer_url");
			bidPrice_ = as_record_get_str(&record, "bid_price");
		}

		as_record * SourceRecord::record()
		{
			if (!record_) {
				record_ = as_record_new(11);

				as_record_set_int64(record_, "latest_time", time_);
				as_record_set_str(record_, "adv_id", advId_.c_str());
				as_record_set_str(record_, "sid", sid_.c_str());
				as_record_set_str(record_, "adx_id", adxId_.c_str());
				as_record_set_str(record_, "mt_uid", mtUid_.c_str());
				as_record_set_str(record_, "pid", pid_.c_str());
				as_record_set_str(record_, "adxpid", adxPid_.c_str());
				as_record_set_str(record_, "request_id", requestId_.c_str());
				as_record_set_str(record_, "create_id", createId_.c_str());
				as_record_set_str(record_, "geo_id", geoId_.c_str());
				as_record_set_str(record_, "referer_url", refererUrl_.c_str());
				as_record_set_str(record_, "bid_price", bidPrice_.c_str());
			}

			return record_;
		}

		int64_t SourceRecord::time() const
		{
			return time_;
		}
		const std::string & SourceRecord::advId() const
		{
			return advId_;
		}
		const std::string & SourceRecord::sid() const
		{
			return sid_;
		}
		const std::string & SourceRecord::adxId() const
		{
			return adxId_;
		}
		const std::string & SourceRecord::mtUid() const
		{
			return mtUid_;
		}
		const std::string & SourceRecord::pid() const
		{
			return pid_;
		}
		const std::string & SourceRecord::adxPid() const
		{
			return adxPid_;
		}
		const std::string & SourceRecord::requestId() const
		{
			return requestId_;
		}
		const std::string & SourceRecord::createId() const
		{
			return createId_;
		}
		const std::string & SourceRecord::geoId() const
		{
			return geoId_;
		}
		const std::string & SourceRecord::refererUrl() const
		{
			return refererUrl_;
		}
		const std::string & SourceRecord::bidPrice() const
		{
			return bidPrice_;
		}

	} // namespace model
} // namespace core
} // namespace adservice
