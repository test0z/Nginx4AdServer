#include "core_typetable.h"
#include "common/atomic.h"
#include "common/functions.h"
#include "logging.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/variant.hpp>
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
		FILE * file = fopen(contenttypefile, "r");
		if (!file) {
			LOG_ERROR << "TypeTableManager::loadContentTypeData areafile can not be opened! file:" << contenttypefile;
			return;
		}
		char buffer[256];
		while (fgets(buffer, sizeof(buffer), file) != NULL) {
			ContenttypeItem item;
			processContentTypeDictFile(buffer, item);
			std::string key = std::to_string(item.adxid);
			key += ":";
			key += item.adxContentType;
			LOG_DEBUG << key;
			ContentTypeDictAccessor acc;
			contentTypeDict.insert(acc, key);
			acc->second = item;
		}
		fclose(file);
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
		ContentTypeDict::const_accessor acc;
		if (contentTypeDict.find(acc, key)) {
			const ContenttypeItem & item = acc->second;
			return item.mttyContentType;
		} else {
			return 0;
		}
	}
}
}
