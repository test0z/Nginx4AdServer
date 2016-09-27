#include "addatamanager.h"
#include "utility/aero_spike.h"

namespace adservice {
namespace adselectv2 {

    using namespace adservice::utility;

    AdDataManager::AdDataManagerPtr AdDataManager::instance = nullptr;

    AdDataManager::AdDataManager(const AdDataConfig& addataConfig)
    {
        this->asNamespace = addataConfig.addataNamespace;
        this->asAdplaceSet = addataConfig.adplaceSet;
        this->asBannerSet = addataConfig.bannerSet;
    }

    bool AdDataManager::getBanner(int64_t bannerId, AdBanner & banner)
    {
        AeroSpike & as = AeroSpike::instance;
        as.get(asNamespace.data(), asBannerSet.data(), bannerId, banner);
        return banner.isValid();
    }

    bool AdDataManager::getAdplace(int64_t adplaceId, AdAdplace & adplace)
    {
        AeroSpike & as = AeroSpike::instance;
        as.get(asNamespace.data(), asAdplaceSet.data(), adplaceId, adplace);
        return adplace.isValid();
    }
}
}
