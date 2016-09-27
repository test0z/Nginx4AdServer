#ifndef ADDATAMANAGER_H
#define ADDATAMANAGER_H

#include "addata.h"
#include "core/config_types.h"
#include <memory>

namespace adservice {

namespace adselectv2 {


    class AdDataManager {
    public:
        typedef std::shared_ptr<AdDataManager> AdDataManagerPtr;
        static AdDataManagerPtr instance;
        static AdDataManagerPtr& getInstance()
        {
            return instance;
        }

        AdDataManager(const AdDataConfig& addataConfig);
        ~AdDataManager()
        {
        }

        bool getBanner(int64_t bannerId, AdBanner & banner);
        bool getAdplace(int64_t adplaceId, AdAdplace & adplace);

    private:
        std::string asNamespace;
        std::string asBannerSet;
        std::string asAdplaceSet;
    };

}
}

#endif // ADDATAMANAGER_H
