//
// Created by guoze.lin on 16/6/27.
//

#ifndef ADCORE_NETEASE_BIDDING_HANDLER_H
#define ADCORE_NETEASE_BIDDING_HANDLER_H

#include <map>

#include "protocol/base/abstract_bidding_handler.h"
#include "utility/json.h"

namespace protocol {
namespace bidding {

	struct NetEaseAdplaceStyle {
		int width, height;
		NetEaseAdplaceStyle()
		{
		}
		NetEaseAdplaceStyle(int w, int h)
			: width(w)
			, height(h)
		{
		}
	};

	class NetEaseAdplaceStyleMap {
	public:
		NetEaseAdplaceStyleMap()
		{
			add(1, 1, 0);
			add(11, 1, 0);
			add(12, 1, 0);
			add(3, 3, 0);
			add(4, 1, 0);
			add(2, 2, 0);
			add(21, 2, 0);
			add(5, 2, 0);
		}
		inline void add(int key, int width, int height)
		{
			styleMap.insert(std::make_pair(key, NetEaseAdplaceStyle(width, height)));
		}
		inline NetEaseAdplaceStyle & get(int key)
		{
			return styleMap[key];
		}
		inline bool find(int key)
		{
			return styleMap.find(key) != styleMap.end();
		}

	private:
		std::map<int, NetEaseAdplaceStyle> styleMap;
	};

	class NetEaseBiddingHandler : public AbstractBiddingHandler {
	public:
		/**
		 * 从Adx Bid Post请求数据中获取具体的请求信息
		 */
		bool parseRequestData(const std::string & data);

		/**
		 * 根据Bid 的相关信息对日志进行信息填充
		 */
		bool fillLogItem(protocol::log::LogItem & logItem);

		/**
		 * 根据ADX的请求进行竞价匹配,决定是否接受这个流量,同时设置isBidAccepted
		 * @return: true接受流量,false不接受流量
		 */
		bool filter(const BiddingFilterCallback & filterCb);

		/**
		 * 将匹配结果转换为具体平台的格式的结果
		 */
		void buildBidResult(const AdSelectCondition & queryCondition, const MT::common::SelectResult & result);

		/**
		 * 当接受流量时装配合适的输出
		 */
		void match(INOUT adservice::utility::HttpResponse & response);

		/**
		 * 不接受ADX的流量请求
		 */
		void reject(INOUT adservice::utility::HttpResponse & response);

	private:
		cppcms::json::value bidRequest;
		cppcms::json::value bidResponse;
	};
}
}

#endif // ADCORE_NETEASE_BIDDING_HANDLER_H
