//
// Created by guoze.lin on 16/5/11.
//

#ifndef ADCORE_GDT_BIDDING_HANDLER_H
#define ADCORE_GDT_BIDDING_HANDLER_H

#include <map>

#include "protocol/base/abstract_bidding_handler.h"
#include "protocol/tencent_gdt/gdt.pb.h"

namespace protocol {
namespace bidding {

	struct GdtAdplace {
		int width, height, flowType;
		GdtAdplace()
		{
		}
		GdtAdplace(int w, int h, int f)
			: width(w)
			, height(h)
			, flowType(f)
		{
		}
	};

	class GdtAdplaceMap {
	public:
		GdtAdplaceMap()
		{
			add(133, 582, 166, SOLUTION_FLOWTYPE_MOBILE);
			add(134, 114, 114, SOLUTION_FLOWTYPE_MOBILE);
			add(79, 640, 960, SOLUTION_FLOWTYPE_MOBILE);
			add(80, 320, 480, SOLUTION_FLOWTYPE_MOBILE);
			add(119, 480, 720, SOLUTION_FLOWTYPE_MOBILE);
			add(120, 640, 1136, SOLUTION_FLOWTYPE_MOBILE);
			add(124, 720, 1280, SOLUTION_FLOWTYPE_MOBILE);
			add(125, 1080, 1920, SOLUTION_FLOWTYPE_MOBILE);
			add(142, 750, 1334, SOLUTION_FLOWTYPE_MOBILE);
			add(96, 140, 425, SOLUTION_FLOWTYPE_PC);
			add(99, 200, 162, SOLUTION_FLOWTYPE_PC);
			add(23, 198, 100, SOLUTION_FLOWTYPE_PC);
			add(12, 198, 40, SOLUTION_FLOWTYPE_PC);
			add(2, 160, 210, SOLUTION_FLOWTYPE_PC);
			add(207, 640, 1136, SOLUTION_FLOWTYPE_MOBILE);
			add(208, 640, 960, SOLUTION_FLOWTYPE_MOBILE);
			add(212, 320, 480, SOLUTION_FLOWTYPE_MOBILE);
			add(147, 1200, 800, SOLUTION_FLOWTYPE_MOBILE);
			add(148, 1280, 720, SOLUTION_FLOWTYPE_MOBILE);
			add(149, 1200, 627, SOLUTION_FLOWTYPE_MOBILE);
			add(150, 800, 1200, SOLUTION_FLOWTYPE_MOBILE);
			add(58, 300, 250, SOLUTION_FLOWTYPE_MOBILE);
			add(59, 600, 500, SOLUTION_FLOWTYPE_MOBILE);
			add(70, 72, 72, SOLUTION_FLOWTYPE_MOBILE);
			add(113, 640, 960, SOLUTION_FLOWTYPE_MOBILE);
			add(114, 320, 480, SOLUTION_FLOWTYPE_MOBILE);
			add(10, 640, 100, SOLUTION_FLOWTYPE_MOBILE);
			add(28, 240, 38, SOLUTION_FLOWTYPE_MOBILE);
			add(31, 480, 75, SOLUTION_FLOWTYPE_MOBILE);
			add(35, 320, 50, SOLUTION_FLOWTYPE_MOBILE);
			add(69, 72, 72, SOLUTION_FLOWTYPE_MOBILE);
			add(65, 1000, 560, SOLUTION_FLOWTYPE_MOBILE);
			add(184, 240, 180, SOLUTION_FLOWTYPE_MOBILE);
			add(210, 640, 288, SOLUTION_FLOWTYPE_MOBILE);
		}
		inline void add(int k, int w, int h, int f)
		{
			gdtMap.insert(std::make_pair(k, GdtAdplace(w, h, f)));
		}
		inline GdtAdplace & get(int k)
		{
			return gdtMap[k];
		}
		inline bool find(int k)
		{
			return gdtMap.find(k) != gdtMap.end();
		}

	private:
		std::map<int, GdtAdplace> gdtMap;
	};

	class GdtSizeMap {
	public:
		GdtSizeMap()
		{
			add(std::make_pair(240, 180), std::make_pair(1, 1));
			add(std::make_pair(1200, 800), std::make_pair(2, 1));
			add(std::make_pair(1280, 720), std::make_pair(2, 2));
			add(std::make_pair(1200, 627), std::make_pair(2, 3));
			add(std::make_pair(800, 1200), std::make_pair(2, 4));
			add(std::make_pair(640, 288), std::make_pair(2, 5));
			add(std::make_pair(1000, 560), std::make_pair(2, 6));
		}
		void add(const std::pair<int, int> & k, const std::pair<int, int> & v)
		{
			sizemap.insert(std::make_pair(k, v));
		}

		std::pair<int, int> get(const std::pair<int, int> & k)
		{
			if (sizemap.find(k) != sizemap.end()) {
				return sizemap[k];
			} else {
				return k;
            }
		}

	private:
		std::map<std::pair<int, int>, std::pair<int, int>> sizemap;
	};

	class GdtBiddingHandler : public AbstractBiddingHandler {
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
		protocol::gdt::adx::BidRequest bidRequest;
		protocol::gdt::adx::BidResponse bidResponse;
	};
}
}

#endif // ADCORE_GDT_BIDDING_HANDLER_H
