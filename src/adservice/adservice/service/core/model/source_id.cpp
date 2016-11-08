#include "source_id.h"

#include <iomanip>

#include "mtty/constants.h"

namespace adservice {
namespace core {
	namespace model {

		const std::string & SourceId::get() const
		{
			return val_;
		}
		void SourceId::record(const as_record * record)
		{
			std::stringstream ss;
			ss << std::setw(12) << std::setfill('0') << std::setbase(16) << as_record_get_int64(record, "val", -1);
			ss >> val_;

			if (record_ == nullptr) {
				record_ = as_record_new(1);
			}
			as_record_set_str(record_, "source_id", val_.c_str());
		}

		SourceRecord::SourceRecord(utility::url::ParamMap & paramMap, protocol::log::LogItem & log)
		{
			record_ = as_record_new(11);

			as_record_set_int64(record_, "latest_time", log.timeStamp);
			as_record_set_str(record_, "adv_id", paramMap[URL_ADOWNER_ID].c_str());
			as_record_set_str(record_, "sid", paramMap[URL_EXEC_ID].c_str());
			as_record_set_str(record_, "adx_id", paramMap[URL_ADX_ID].c_str());
			as_record_set_str(record_, "mt_uid", log.userId.c_str());
			as_record_set_str(record_, "pid", paramMap[URL_MTTYADPLACE_ID].c_str());
			as_record_set_str(record_, "adxpid", paramMap[URL_ADPLACE_ID].c_str());
			as_record_set_str(record_, "request_id", paramMap[URL_EXPOSE_ID].c_str());
			as_record_set_str(record_, "create_id", paramMap[URL_CREATIVE_ID].c_str());
			as_record_set_str(record_, "geo_id", paramMap[URL_AREA_ID].c_str());
			as_record_set_str(record_, "referer_url", paramMap[URL_REFERER].c_str());
			as_record_set_str(record_, "bid_price", paramMap[URL_BID_PRICE].c_str());
		}

		void SourceRecord::record(const as_record * record)
		{
			time_ = as_record_get_int64(record, "latest_time", -1);
			advId_ = getStr("adv_id");
			sid_ = getStr("sid");
			adxId_ = getStr("adx_id");
			mtUid_ = getStr("mt_uid");
			pid_ = getStr("pid");
			adxPid_ = getStr("adxpid");
			requestId_ = getStr("request_id");
			createId_ = getStr("create_id");
			geoId_ = getStr("geo_id");
			refererUrl_ = getStr("referer_url");
			bidPrice_ = getStr("bid_price");
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
