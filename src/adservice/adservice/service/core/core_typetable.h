#ifndef CORE_TYPETABLE_H
#define CORE_TYPETABLE_H

#include "tbb/concurrent_hash_map.h"
#include "common/constants.h"

namespace adservice{
namespace server{

    class ContenttypeItem{
    public:
        int adxid;
        std::string adxContentType;
        int mttyContentType;
    };

    typedef tbb::concurrent_hash_map<std::string,ContenttypeItem> ContentTypeDict;
    typedef ContentTypeDict::accessor ContentTypeDictAccessor;

    class TypeTableManager{
    public:
        static void init(const char* contenttypeFile = CONTENTTYPE_FILE);
        static void destroy();
        static void loadContentTypeData(const char* areafile);
        static TypeTableManager& getInstance(){
            static TypeTableManager manager;
            if(started==0)
                manager.init();
            return manager;
        }
        static ContentTypeDict contentTypeDict;
        static int started;
    public:
        TypeTableManager(){}
        TypeTableManager(const TypeTableManager&) = delete;
        int getContentType(int adxId,const std::string& adxContentType);
    };

}
}

#endif // CORE_TYPETABLE_H
