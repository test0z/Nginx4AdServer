#include "core_typetable.h"
#include "common/atomic.h"
#include "common/functions.h"
#include "logging.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/variant.hpp>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <vector>

namespace adservice {
namespace server {

    ContentTypeDict TypeTableManager::contentTypeDict;
    int TypeTableManager::started = 0;

    namespace {
        void processContentTypeDictFile(const char * line, ContenttypeItem & item)
        {
            std::vector<std::string> tmp;
            boost::split(tmp, line, boost::is_any_of("|"));
            if (tmp.size() != 3) {
                throw "process ContentType Dict File";
            }
            item.adxid = std::stoi(tmp[0]);
            item.adxContentType = tmp[1];
            item.mttyContentType = std::stoi(tmp[2]);
        }
    }

    void TypeTableManager::loadContentTypeData(const char * contenttypefile)
    {
        try {
            sql::Driver * driver;
            sql::Connection * con;
            sql::Statement * stmt;
            sql::ResultSet * res;
            driver = get_driver_instance();
            con = driver->connect("tcp://rm-2zek1ct2g06ergoum.mysql.rds.aliyuncs.com:3306", "mtty_root", "Mtkj*8888");
            con->setSchema("mtdb");
            stmt = con->createStatement();
            res = stmt->executeQuery("select adxid,adx_category_id,common_category_id from content_category_map ");

            while (res->next()) {
                int64_t adxId = res->getInt64("adxid");
                std::string adxCategory = res->getString("adx_category_id");
                std::string ourCategory = res->getString("common_category_id");
                ContenttypeItem item;
                item.adxid = adxId;
                item.adxContentType = adxCategory;
                item.mttyContentType = std::stoi(ourCategory);
                contentTypeDict.insert({ std::to_string(adxId) + ":" + adxCategory, item });
            }
            delete res;
            delete stmt;
            delete con;
        } catch (sql::SQLException & e) {
            std::cerr << "sql::SQLException:" << e.what() << ",errorcode:" << e.getErrorCode()
                      << ",sqlstate:" << e.getSQLState() << std::endl;
            FILE * file = fopen(contenttypefile, "r");
            if (!file) {
                LOG_ERROR << "TypeTableManager::loadContentTypeData areafile can not be opened! file:"
                          << contenttypefile;
                return;
            }
            char buffer[256];
            while (fgets(buffer, sizeof(buffer), file) != NULL) {
                ContenttypeItem item;
                processContentTypeDictFile(buffer, item);
                std::string key = std::to_string(item.adxid);
                key += ":";
                key += item.adxContentType;
                // LOG_DEBUG << key;
                contentTypeDict.insert({ key, item });
            }
            fclose(file);
        }
    }

    void TypeTableManager::init(const char * contentTypeFile)
    {
        if (!ATOM_CAS(&started, 0, 1)) {
            return;
        }
        loadContentTypeData(contentTypeFile);
    }

    void TypeTableManager::destroy()
    {
        if (!ATOM_CAS(&started, 1, 0)) {
            return;
        }
    }

    int TypeTableManager::getContentType(int adxId, const std::string & adxContentType)
    {
        std::string key = std::to_string(adxId);
        key += ":";
        key += adxContentType;
        auto iter = contentTypeDict.find(key);
        if (iter != contentTypeDict.end()) {
            const ContenttypeItem & item = iter->second;
            return item.mttyContentType;
        } else {
            return 0;
        }
    }
}
}
