#include "addata.h"

namespace adservice{
namespace adselectv2{



    void AdBanner::parse(const as_record *record){
        this->bannerId = as_record_get_int64(record,"bid",0);
        this->bannerStatus = as_record_get_int64(record,"bannerstatus",2);
        this->width = as_record_get_int64(record,"width",0);
        this->height = as_record_get_int64(record,"height",0);
        this->bannerType = as_record_get_int64(record,"bannertype",0);
        this->offerPrice = as_record_get_double(record,"offerprice",0.0);
        this->ctr = as_record_get_double(record,"ctr",0.0);
        this->json = as_record_get_str(record,"json");
        this->adxAdvId = as_record_get_str(record,"adxadvid");
        this->adxIndustryType = as_record_get_str(record,"adxidstype");
    }

    void AdAdplace::parse(const as_record *record){
        this->adplaceId = as_record_get_int64(record,"pid",0);
        this->adxId = as_record_get_int64(record,"adxid",0);
        this->adxPid = as_record_get_str(record,"adxpid");
        this->cid = as_record_get_int64(record,"cid",0);
        this->mid = as_record_get_int64(record,"mid",0);
        this->costPrice = as_record_get_int64(record,"costPrice",0);
        this->mediaType = as_record_get_int64(record,"mediatype",0);
        this->basePrice = as_record_get_double(record,"baseprice",0.0);
    }


}
}
