//
// Created by guoze.lin on 16/5/3.
//

#ifndef ADCORE_BID_QUERY_TASK_H
#define ADCORE_BID_QUERY_TASK_H

#include <initializer_list>

#include "abstract_query_task.h"
#include "core/core_ip_manager.h"
#include "protocol/base/abstract_bidding_handler.h"

namespace adservice {
namespace corelogic {

    using namespace protocol::bidding;

    static const int BID_MAX_MODULES = 10;

    struct ModuleIndex {
		uint64_t moduleHash;
        int64_t adxId;

		ModuleIndex() = default;

		ModuleIndex(uint64_t h, int64_t id)
			: moduleHash(h)
			, adxId(id)
		{
        }
    };

    /**
     * 处理竞价模块逻辑的类
     */
    class HandleBidQueryTask : public AbstractQueryTask {
    public:
        static int initialized;
        static int moduleCnt;
        static struct ModuleIndex moduleAdx[BID_MAX_MODULES];
        static int moduleIdx[BID_MAX_MODULES];
        static void init();
        static int getAdxId(const std::string & path);
        static AbstractBiddingHandler * getBiddingHandler(int adxId);

    public:
		explicit HandleBidQueryTask(adservice::utility::HttpRequest & request,
									adservice::utility::HttpResponse & response)
            : AbstractQueryTask(request, response)
        {
            adxId = getAdxId(request.path_info());
            biddingHandler = NULL;
        }

        void updateBiddingHandler();

        protocol::log::LogPhaseType currentPhase()
        {
            return protocol::log::LogPhaseType::BID;
        }

        // 期望http 请求状态
        int expectedReqStatus()
        {
            return 300;
        }

        void getPostParam(ParamMap & paramMap);

		void customLogic(ParamMap & paramMap, protocol::log::LogItem & log,
						 adservice::utility::HttpResponse & response);

        virtual void onError(std::exception & e, adservice::utility::HttpResponse & resp) override;

    private:
        int adxId;
        AbstractBiddingHandler * biddingHandler;
    };
}
}

#endif // ADCORE_BID_QUERY_TASK_H
