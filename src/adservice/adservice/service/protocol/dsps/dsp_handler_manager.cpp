#include "dsp_handler_manager.h"
#include "core/config_types.h"
#include "utility/utility.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <string>

extern GlobalConfig globalConfig;

namespace protocol {
namespace dsp {

    thread_local DSPHandlerManager dspHandlerManager;

    namespace {

        struct DSPInfo {
            int64_t dspId;
            std::string dspBidUrl;
            std::string dspCmUrl;
        };

        std::vector<DSPInfo> initFromDB()
        {
            std::vector<DSPInfo> dspInfos;
            DBConfig & dbConfig = globalConfig.dbConfig;
            try {
                sql::Driver * driver;
                sql::Connection * con;
                sql::Statement * stmt;
                sql::ResultSet * res;
                driver = get_driver_instance();
                con = driver->connect(dbConfig.accessUrl, dbConfig.userName, dbConfig.password);
                con->setSchema(dbConfig.dbName);
                stmt = con->createStatement();
                res = stmt->executeQuery("select id,bidurl,'' as cmurl from sys_user where roleid=11");

                while (res->next()) {
                    DSPInfo dspInfo;
                    dspInfo.dspId = res->getInt64("id");
                    dspInfo.dspBidUrl = res->getString("bidurl");
                    dspInfo.dspCmUrl = res->getString("cmurl");
                    dspInfos.push_back(dspInfo);
                }
                delete res;
                delete stmt;
                delete con;
            } catch (sql::SQLException & e) {
                std::cerr << "sql::SQLException:" << e.what() << ",errorcode:" << e.getErrorCode()
                          << ",sqlstate:" << e.getSQLState() << std::endl;
            }
            dspInfos.push_back({ 360L, "http://127.0.0.1:20007/mttybid", "http://127.0.0.1:20007/m" });
            return dspInfos;
        }

        std::shared_ptr<DefaultDSPHandler> getDSPHandler(int64_t dspId, std::string bidUrl, std::string cmUrl)
        {
            switch (dspId) {
            default:
                return std::make_shared<DefaultDSPHandler>(dspId, bidUrl, cmUrl, 200);
            }
        }
    }

    DSPHandlerManager::DSPHandlerManager()
        : executor("DSPExecutor")
    {
        std::vector<DSPInfo> dspInfos = std::move(initFromDB());
        for (DSPInfo & dspInfo : dspInfos) {
            if (!dspInfo.dspBidUrl.empty()) {
                adservice::utility::url::url_replace(dspInfo.dspBidUrl, "https://", "http://");
                LOG_DEBUG << "add dsp handler,dspId:" << dspInfo.dspId << ",bidurl:" << dspInfo.dspBidUrl
                          << ",cmurl:" << dspInfo.dspCmUrl;
                addHandler(dspInfo.dspId, getDSPHandler(dspInfo.dspId, dspInfo.dspBidUrl, dspInfo.dspCmUrl));
            } else {
                LOG_WARN << "dsp " << dspInfo.dspId << " bidurl empty";
            }
        }
        executor.start();
    }

    DSPHandlerInterfacePtr DSPHandlerManager::getHandler(int64_t dspId)
    {
        auto iter = handlers.find(dspId);
        if (iter != handlers.end()) {
            return iter->second;
        }
        throw DSPHandlerException(std::to_string(dspId) + " handler not Found");
    }
}
}
