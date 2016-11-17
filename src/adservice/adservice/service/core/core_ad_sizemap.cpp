#include "core_ad_sizemap.h"


namespace adservice {
namespace utility{

    AdSizeMap AdSizeMap::instance_;

    const AdSizeMap& AdSizeMap::getInstance(){
        return instance_;
    }

}
}
