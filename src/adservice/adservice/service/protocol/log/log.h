/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LOG_AVRO_LOG_H_1312058860__H_
#define LOG_AVRO_LOG_H_1312058860__H_

#include "avro/Decoder.hh"
#include "avro/Encoder.hh"
#include "avro/Specific.hh"
#include "boost/any.hpp"
#include <sstream>

namespace protocol {
namespace log {
	struct GeoInfo {
		std::string latitude;
		std::string longitude;
		int32_t country;
		int32_t province;
		int32_t city;
		int32_t district;
		int32_t street;
		GeoInfo()
			: latitude(std::string())
			, longitude(std::string())
			, country(int32_t())
			, province(int32_t())
			, city(int32_t())
			, district(int32_t())
			, street(int32_t())
		{
		}
	};

	struct AdInfo {
		std::string areaId;
		int64_t advId;
		int64_t cpid;
		int64_t sid;
		int64_t bannerId;
		std::string clickId;
		int32_t adxid;
		int64_t mid;
		int64_t cid;
		std::string pid;
		std::string imp_id;
		std::string landingUrl;
		int32_t cost;
		int32_t bidPrice;
		std::string adxpid;
		std::string adxuid;
		int32_t offerPrice;
		std::string bidSize;
		int32_t priceType;
		int32_t ppid;
		int64_t orderId;
		std::vector<int64_t> ppids;
		AdInfo()
			: areaId(std::string())
			, advId(int64_t())
			, cpid(int64_t())
			, sid(int64_t())
			, bannerId(int64_t())
			, clickId(std::string())
			, adxid(int32_t())
			, mid(int64_t())
			, cid(int64_t())
			, pid(std::string())
			, imp_id(std::string())
			, landingUrl(std::string())
			, cost(int32_t())
			, bidPrice(int32_t())
			, adxpid(std::string())
			, adxuid(std::string())
			, offerPrice(int32_t())
			, bidSize(std::string())
			, priceType(int32_t())
			, ppid(int32_t())
			, orderId(int64_t())
			, ppids(std::vector<int64_t>())
		{
		}
	};

	enum LogPhaseType {
		BID,
		SHOW,
		VIEW,
		CLICK,
		TRACK,
		MAPPING,
	};

	struct IPInfo {
		int32_t ipv4;
		std::vector<int32_t> ipv6;
		std::string proxy;
		IPInfo()
			: ipv4(int32_t())
			, ipv6(std::vector<int32_t>())
			, proxy(std::string())
		{
		}
	};

	struct UserInfo {
		int32_t age;
		int32_t sex;
		int32_t interest;
		UserInfo()
			: age(int32_t())
			, sex(int32_t())
			, interest(int32_t())
		{
		}
	};

	struct TraceInfo {
		std::string version;
		std::string deviceType;
		std::string sourceid;
		std::string tag1;
		std::string tag2;
		std::string tag3;
		std::string tag4;
		std::string tag5;
		std::string tag6;
		std::string tag7;
		std::string tag8;
		std::string tag9;
		std::string tag10;
		TraceInfo()
			: version(std::string())
			, deviceType(std::string())
			, sourceid(std::string())
			, tag1(std::string())
			, tag2(std::string())
			, tag3(std::string())
			, tag4(std::string())
			, tag5(std::string())
			, tag6(std::string())
			, tag7(std::string())
			, tag8(std::string())
			, tag9(std::string())
			, tag10(std::string())
		{
		}
	};

	struct LogItem {
		int64_t timeStamp;
		LogPhaseType logType;
		int32_t reqStatus;
		int32_t reqMethod;
		IPInfo ipInfo;
		std::string referer;
		std::string host;
		std::string path;
		std::string userId;
		std::string userAgent;
		UserInfo userInfo;
		GeoInfo geoInfo;
		std::string pageInfo;
		std::string jsInfo;
		std::string deviceInfo;
		int32_t traceId;
		AdInfo adInfo;
		int32_t clickx;
		int32_t clicky;
		TraceInfo traceInfo;
		LogItem()
			: timeStamp(int64_t())
			, logType(LogPhaseType())
			, reqStatus(int32_t())
			, reqMethod(int32_t())
			, ipInfo(IPInfo())
			, referer(std::string())
			, host(std::string())
			, path(std::string())
			, userId(std::string())
			, userAgent(std::string())
			, userInfo(UserInfo())
			, geoInfo(GeoInfo())
			, pageInfo(std::string())
			, jsInfo(std::string())
			, deviceInfo(std::string())
			, traceId(int32_t())
			, adInfo(AdInfo())
			, clickx(int32_t())
			, clicky(int32_t())
			, traceInfo(TraceInfo())
		{
		}
	};

	struct __tmp_json_Union__0__ {
	private:
		size_t idx_;
		boost::any value_;

	public:
		size_t idx() const
		{
			return idx_;
		}
		GeoInfo get_GeoInfo() const;
		void set_GeoInfo(const GeoInfo & v);
		AdInfo get_AdInfo() const;
		void set_AdInfo(const AdInfo & v);
		LogPhaseType get_LogPhaseType() const;
		void set_LogPhaseType(const LogPhaseType & v);
		IPInfo get_IPInfo() const;
		void set_IPInfo(const IPInfo & v);
		UserInfo get_UserInfo() const;
		void set_UserInfo(const UserInfo & v);
		TraceInfo get_TraceInfo() const;
		void set_TraceInfo(const TraceInfo & v);
		LogItem get_LogItem() const;
		void set_LogItem(const LogItem & v);
		__tmp_json_Union__0__();
	};

	inline GeoInfo __tmp_json_Union__0__::get_GeoInfo() const
	{
		if (idx_ != 0) {
			throw avro::Exception("Invalid type for union");
		}
		return boost::any_cast<GeoInfo>(value_);
    }

	inline void __tmp_json_Union__0__::set_GeoInfo(const GeoInfo & v)
	{
		idx_ = 0;
		value_ = v;
	}

	inline AdInfo __tmp_json_Union__0__::get_AdInfo() const
	{
		if (idx_ != 1) {
			throw avro::Exception("Invalid type for union");
		}
		return boost::any_cast<AdInfo>(value_);
    }

	inline void __tmp_json_Union__0__::set_AdInfo(const AdInfo & v)
	{
		idx_ = 1;
		value_ = v;
	}

	inline LogPhaseType __tmp_json_Union__0__::get_LogPhaseType() const
	{
		if (idx_ != 2) {
			throw avro::Exception("Invalid type for union");
		}
		return boost::any_cast<LogPhaseType>(value_);
    }

	inline void __tmp_json_Union__0__::set_LogPhaseType(const LogPhaseType & v)
	{
		idx_ = 2;
		value_ = v;
	}

	inline IPInfo __tmp_json_Union__0__::get_IPInfo() const
	{
		if (idx_ != 3) {
			throw avro::Exception("Invalid type for union");
		}
		return boost::any_cast<IPInfo>(value_);
    }

	inline void __tmp_json_Union__0__::set_IPInfo(const IPInfo & v)
	{
		idx_ = 3;
		value_ = v;
	}

	inline UserInfo __tmp_json_Union__0__::get_UserInfo() const
	{
		if (idx_ != 4) {
			throw avro::Exception("Invalid type for union");
		}
		return boost::any_cast<UserInfo>(value_);
    }

	inline void __tmp_json_Union__0__::set_UserInfo(const UserInfo & v)
	{
		idx_ = 4;
		value_ = v;
	}

	inline TraceInfo __tmp_json_Union__0__::get_TraceInfo() const
	{
		if (idx_ != 5) {
			throw avro::Exception("Invalid type for union");
		}
		return boost::any_cast<TraceInfo>(value_);
    }

	inline void __tmp_json_Union__0__::set_TraceInfo(const TraceInfo & v)
	{
		idx_ = 5;
		value_ = v;
	}

	inline LogItem __tmp_json_Union__0__::get_LogItem() const
	{
		if (idx_ != 6) {
			throw avro::Exception("Invalid type for union");
		}
		return boost::any_cast<LogItem>(value_);
    }

	inline void __tmp_json_Union__0__::set_LogItem(const LogItem & v)
	{
		idx_ = 6;
		value_ = v;
	}

	inline __tmp_json_Union__0__::__tmp_json_Union__0__()
		: idx_(0)
		, value_(GeoInfo())
	{
	}
}
}
namespace avro {
template <>
struct codec_traits<protocol::log::GeoInfo> {
	static void encode(Encoder & e, const protocol::log::GeoInfo & v)
	{
        avro::encode(e, v.latitude);
        avro::encode(e, v.longitude);
        avro::encode(e, v.country);
        avro::encode(e, v.province);
        avro::encode(e, v.city);
        avro::encode(e, v.district);
        avro::encode(e, v.street);
    }
	static void decode(Decoder & d, protocol::log::GeoInfo & v)
	{
		if (avro::ResolvingDecoder * rd = dynamic_cast<avro::ResolvingDecoder *>(&d)) {
            const std::vector<size_t> fo = rd->fieldOrder();
			for (std::vector<size_t>::const_iterator it = fo.begin(); it != fo.end(); ++it) {
                switch (*it) {
                case 0:
                    avro::decode(d, v.latitude);
                    break;
                case 1:
                    avro::decode(d, v.longitude);
                    break;
                case 2:
                    avro::decode(d, v.country);
                    break;
                case 3:
                    avro::decode(d, v.province);
                    break;
                case 4:
                    avro::decode(d, v.city);
                    break;
                case 5:
                    avro::decode(d, v.district);
                    break;
                case 6:
                    avro::decode(d, v.street);
                    break;
                default:
                    break;
                }
            }
        } else {
            avro::decode(d, v.latitude);
            avro::decode(d, v.longitude);
            avro::decode(d, v.country);
            avro::decode(d, v.province);
            avro::decode(d, v.city);
            avro::decode(d, v.district);
            avro::decode(d, v.street);
        }
    }
};

template <>
struct codec_traits<protocol::log::AdInfo> {
	static void encode(Encoder & e, const protocol::log::AdInfo & v)
	{
        avro::encode(e, v.areaId);
        avro::encode(e, v.advId);
        avro::encode(e, v.cpid);
        avro::encode(e, v.sid);
        avro::encode(e, v.bannerId);
        avro::encode(e, v.clickId);
        avro::encode(e, v.adxid);
        avro::encode(e, v.mid);
        avro::encode(e, v.cid);
        avro::encode(e, v.pid);
        avro::encode(e, v.imp_id);
        avro::encode(e, v.landingUrl);
        avro::encode(e, v.cost);
        avro::encode(e, v.bidPrice);
        avro::encode(e, v.adxpid);
        avro::encode(e, v.adxuid);
        avro::encode(e, v.offerPrice);
        avro::encode(e, v.bidSize);
        avro::encode(e, v.priceType);
        avro::encode(e, v.ppid);
        avro::encode(e, v.orderId);
        avro::encode(e, v.ppids);
    }
	static void decode(Decoder & d, protocol::log::AdInfo & v)
	{
		if (avro::ResolvingDecoder * rd = dynamic_cast<avro::ResolvingDecoder *>(&d)) {
            const std::vector<size_t> fo = rd->fieldOrder();
			for (std::vector<size_t>::const_iterator it = fo.begin(); it != fo.end(); ++it) {
                switch (*it) {
                case 0:
                    avro::decode(d, v.areaId);
                    break;
                case 1:
                    avro::decode(d, v.advId);
                    break;
                case 2:
                    avro::decode(d, v.cpid);
                    break;
                case 3:
                    avro::decode(d, v.sid);
                    break;
                case 4:
                    avro::decode(d, v.bannerId);
                    break;
                case 5:
                    avro::decode(d, v.clickId);
                    break;
                case 6:
                    avro::decode(d, v.adxid);
                    break;
                case 7:
                    avro::decode(d, v.mid);
                    break;
                case 8:
                    avro::decode(d, v.cid);
                    break;
                case 9:
                    avro::decode(d, v.pid);
                    break;
                case 10:
                    avro::decode(d, v.imp_id);
                    break;
                case 11:
                    avro::decode(d, v.landingUrl);
                    break;
                case 12:
                    avro::decode(d, v.cost);
                    break;
                case 13:
                    avro::decode(d, v.bidPrice);
                    break;
                case 14:
                    avro::decode(d, v.adxpid);
                    break;
                case 15:
                    avro::decode(d, v.adxuid);
                    break;
                case 16:
                    avro::decode(d, v.offerPrice);
                    break;
                case 17:
                    avro::decode(d, v.bidSize);
                    break;
                case 18:
                    avro::decode(d, v.priceType);
                    break;
                case 19:
                    avro::decode(d, v.ppid);
                    break;
                case 20:
                    avro::decode(d, v.orderId);
                    break;
                case 21:
                    avro::decode(d, v.ppids);
                    break;
                default:
                    break;
                }
            }
        } else {
            avro::decode(d, v.areaId);
            avro::decode(d, v.advId);
            avro::decode(d, v.cpid);
            avro::decode(d, v.sid);
            avro::decode(d, v.bannerId);
            avro::decode(d, v.clickId);
            avro::decode(d, v.adxid);
            avro::decode(d, v.mid);
            avro::decode(d, v.cid);
            avro::decode(d, v.pid);
            avro::decode(d, v.imp_id);
            avro::decode(d, v.landingUrl);
            avro::decode(d, v.cost);
            avro::decode(d, v.bidPrice);
            avro::decode(d, v.adxpid);
            avro::decode(d, v.adxuid);
            avro::decode(d, v.offerPrice);
            avro::decode(d, v.bidSize);
            avro::decode(d, v.priceType);
            avro::decode(d, v.ppid);
            avro::decode(d, v.orderId);
            avro::decode(d, v.ppids);
        }
    }
};

template <>
struct codec_traits<protocol::log::LogPhaseType> {
	static void encode(Encoder & e, protocol::log::LogPhaseType v)
	{
		if (v < protocol::log::BID || v > protocol::log::MAPPING) {
			std::ostringstream error;
			error << "enum value " << v << " is out of bound for protocol::log::LogPhaseType and cannot be encoded";
			throw avro::Exception(error.str());
		}
        e.encodeEnum(v);
    }
	static void decode(Decoder & d, protocol::log::LogPhaseType & v)
	{
		size_t index = d.decodeEnum();
		if (index > protocol::log::MAPPING) {
			std::ostringstream error;
			error << "enum value " << index << " is out of bound for protocol::log::LogPhaseType and cannot be decoded";
			throw avro::Exception(error.str());
		}
        v = static_cast<protocol::log::LogPhaseType>(index);
    }
};

template <>
struct codec_traits<protocol::log::IPInfo> {
	static void encode(Encoder & e, const protocol::log::IPInfo & v)
	{
        avro::encode(e, v.ipv4);
        avro::encode(e, v.ipv6);
        avro::encode(e, v.proxy);
    }
	static void decode(Decoder & d, protocol::log::IPInfo & v)
	{
		if (avro::ResolvingDecoder * rd = dynamic_cast<avro::ResolvingDecoder *>(&d)) {
            const std::vector<size_t> fo = rd->fieldOrder();
			for (std::vector<size_t>::const_iterator it = fo.begin(); it != fo.end(); ++it) {
                switch (*it) {
                case 0:
                    avro::decode(d, v.ipv4);
                    break;
                case 1:
                    avro::decode(d, v.ipv6);
                    break;
                case 2:
                    avro::decode(d, v.proxy);
                    break;
                default:
                    break;
                }
            }
        } else {
            avro::decode(d, v.ipv4);
            avro::decode(d, v.ipv6);
            avro::decode(d, v.proxy);
        }
    }
};

template <>
struct codec_traits<protocol::log::UserInfo> {
	static void encode(Encoder & e, const protocol::log::UserInfo & v)
	{
        avro::encode(e, v.age);
        avro::encode(e, v.sex);
        avro::encode(e, v.interest);
    }
	static void decode(Decoder & d, protocol::log::UserInfo & v)
	{
		if (avro::ResolvingDecoder * rd = dynamic_cast<avro::ResolvingDecoder *>(&d)) {
            const std::vector<size_t> fo = rd->fieldOrder();
			for (std::vector<size_t>::const_iterator it = fo.begin(); it != fo.end(); ++it) {
                switch (*it) {
                case 0:
                    avro::decode(d, v.age);
                    break;
                case 1:
                    avro::decode(d, v.sex);
                    break;
                case 2:
                    avro::decode(d, v.interest);
                    break;
                default:
                    break;
                }
            }
        } else {
            avro::decode(d, v.age);
            avro::decode(d, v.sex);
            avro::decode(d, v.interest);
        }
    }
};

template <>
struct codec_traits<protocol::log::TraceInfo> {
	static void encode(Encoder & e, const protocol::log::TraceInfo & v)
	{
        avro::encode(e, v.version);
        avro::encode(e, v.deviceType);
        avro::encode(e, v.sourceid);
        avro::encode(e, v.tag1);
        avro::encode(e, v.tag2);
        avro::encode(e, v.tag3);
        avro::encode(e, v.tag4);
        avro::encode(e, v.tag5);
        avro::encode(e, v.tag6);
        avro::encode(e, v.tag7);
        avro::encode(e, v.tag8);
        avro::encode(e, v.tag9);
        avro::encode(e, v.tag10);
    }
	static void decode(Decoder & d, protocol::log::TraceInfo & v)
	{
		if (avro::ResolvingDecoder * rd = dynamic_cast<avro::ResolvingDecoder *>(&d)) {
            const std::vector<size_t> fo = rd->fieldOrder();
			for (std::vector<size_t>::const_iterator it = fo.begin(); it != fo.end(); ++it) {
                switch (*it) {
                case 0:
                    avro::decode(d, v.version);
                    break;
                case 1:
                    avro::decode(d, v.deviceType);
                    break;
                case 2:
                    avro::decode(d, v.sourceid);
                    break;
                case 3:
                    avro::decode(d, v.tag1);
                    break;
                case 4:
                    avro::decode(d, v.tag2);
                    break;
                case 5:
                    avro::decode(d, v.tag3);
                    break;
                case 6:
                    avro::decode(d, v.tag4);
                    break;
                case 7:
                    avro::decode(d, v.tag5);
                    break;
                case 8:
                    avro::decode(d, v.tag6);
                    break;
                case 9:
                    avro::decode(d, v.tag7);
                    break;
                case 10:
                    avro::decode(d, v.tag8);
                    break;
                case 11:
                    avro::decode(d, v.tag9);
                    break;
                case 12:
                    avro::decode(d, v.tag10);
                    break;
                default:
                    break;
                }
            }
        } else {
            avro::decode(d, v.version);
            avro::decode(d, v.deviceType);
            avro::decode(d, v.sourceid);
            avro::decode(d, v.tag1);
            avro::decode(d, v.tag2);
            avro::decode(d, v.tag3);
            avro::decode(d, v.tag4);
            avro::decode(d, v.tag5);
            avro::decode(d, v.tag6);
            avro::decode(d, v.tag7);
            avro::decode(d, v.tag8);
            avro::decode(d, v.tag9);
            avro::decode(d, v.tag10);
        }
    }
};

template <>
struct codec_traits<protocol::log::LogItem> {
	static void encode(Encoder & e, const protocol::log::LogItem & v)
	{
        avro::encode(e, v.timeStamp);
        avro::encode(e, v.logType);
        avro::encode(e, v.reqStatus);
        avro::encode(e, v.reqMethod);
        avro::encode(e, v.ipInfo);
        avro::encode(e, v.referer);
        avro::encode(e, v.host);
        avro::encode(e, v.path);
        avro::encode(e, v.userId);
        avro::encode(e, v.userAgent);
        avro::encode(e, v.userInfo);
        avro::encode(e, v.geoInfo);
        avro::encode(e, v.pageInfo);
        avro::encode(e, v.jsInfo);
        avro::encode(e, v.deviceInfo);
        avro::encode(e, v.traceId);
        avro::encode(e, v.adInfo);
        avro::encode(e, v.clickx);
        avro::encode(e, v.clicky);
        avro::encode(e, v.traceInfo);
    }
	static void decode(Decoder & d, protocol::log::LogItem & v)
	{
		if (avro::ResolvingDecoder * rd = dynamic_cast<avro::ResolvingDecoder *>(&d)) {
            const std::vector<size_t> fo = rd->fieldOrder();
			for (std::vector<size_t>::const_iterator it = fo.begin(); it != fo.end(); ++it) {
                switch (*it) {
                case 0:
                    avro::decode(d, v.timeStamp);
                    break;
                case 1:
                    avro::decode(d, v.logType);
                    break;
                case 2:
                    avro::decode(d, v.reqStatus);
                    break;
                case 3:
                    avro::decode(d, v.reqMethod);
                    break;
                case 4:
                    avro::decode(d, v.ipInfo);
                    break;
                case 5:
                    avro::decode(d, v.referer);
                    break;
                case 6:
                    avro::decode(d, v.host);
                    break;
                case 7:
                    avro::decode(d, v.path);
                    break;
                case 8:
                    avro::decode(d, v.userId);
                    break;
                case 9:
                    avro::decode(d, v.userAgent);
                    break;
                case 10:
                    avro::decode(d, v.userInfo);
                    break;
                case 11:
                    avro::decode(d, v.geoInfo);
                    break;
                case 12:
                    avro::decode(d, v.pageInfo);
                    break;
                case 13:
                    avro::decode(d, v.jsInfo);
                    break;
                case 14:
                    avro::decode(d, v.deviceInfo);
                    break;
                case 15:
                    avro::decode(d, v.traceId);
                    break;
                case 16:
                    avro::decode(d, v.adInfo);
                    break;
                case 17:
                    avro::decode(d, v.clickx);
                    break;
                case 18:
                    avro::decode(d, v.clicky);
                    break;
                case 19:
                    avro::decode(d, v.traceInfo);
                    break;
                default:
                    break;
                }
            }
        } else {
            avro::decode(d, v.timeStamp);
            avro::decode(d, v.logType);
            avro::decode(d, v.reqStatus);
            avro::decode(d, v.reqMethod);
            avro::decode(d, v.ipInfo);
            avro::decode(d, v.referer);
            avro::decode(d, v.host);
            avro::decode(d, v.path);
            avro::decode(d, v.userId);
            avro::decode(d, v.userAgent);
            avro::decode(d, v.userInfo);
            avro::decode(d, v.geoInfo);
            avro::decode(d, v.pageInfo);
            avro::decode(d, v.jsInfo);
            avro::decode(d, v.deviceInfo);
            avro::decode(d, v.traceId);
            avro::decode(d, v.adInfo);
            avro::decode(d, v.clickx);
            avro::decode(d, v.clicky);
            avro::decode(d, v.traceInfo);
        }
    }
};

template <>
struct codec_traits<protocol::log::__tmp_json_Union__0__> {
	static void encode(Encoder & e, protocol::log::__tmp_json_Union__0__ v)
	{
        e.encodeUnionIndex(v.idx());
        switch (v.idx()) {
        case 0:
            avro::encode(e, v.get_GeoInfo());
            break;
        case 1:
            avro::encode(e, v.get_AdInfo());
            break;
        case 2:
            avro::encode(e, v.get_LogPhaseType());
            break;
        case 3:
            avro::encode(e, v.get_IPInfo());
            break;
        case 4:
            avro::encode(e, v.get_UserInfo());
            break;
        case 5:
            avro::encode(e, v.get_TraceInfo());
            break;
        case 6:
            avro::encode(e, v.get_LogItem());
            break;
        }
    }
	static void decode(Decoder & d, protocol::log::__tmp_json_Union__0__ & v)
	{
        size_t n = d.decodeUnionIndex();
		if (n >= 7) {
			throw avro::Exception("Union index too big");
		}
        switch (n) {
		case 0: {
			protocol::log::GeoInfo vv;
			avro::decode(d, vv);
			v.set_GeoInfo(vv);
		} break;
		case 1: {
			protocol::log::AdInfo vv;
			avro::decode(d, vv);
			v.set_AdInfo(vv);
		} break;
		case 2: {
			protocol::log::LogPhaseType vv;
			avro::decode(d, vv);
			v.set_LogPhaseType(vv);
		} break;
		case 3: {
			protocol::log::IPInfo vv;
			avro::decode(d, vv);
			v.set_IPInfo(vv);
		} break;
		case 4: {
			protocol::log::UserInfo vv;
			avro::decode(d, vv);
			v.set_UserInfo(vv);
		} break;
		case 5: {
			protocol::log::TraceInfo vv;
			avro::decode(d, vv);
			v.set_TraceInfo(vv);
		} break;
		case 6: {
			protocol::log::LogItem vv;
			avro::decode(d, vv);
			v.set_LogItem(vv);
		} break;
        }
    }
};
}
#endif
