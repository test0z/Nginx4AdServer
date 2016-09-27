#ifndef ADDATA_H
#define ADDATA_H

#include <aerospike/aerospike.h>
#include <aerospike/as_record.h>
#include <boost/serialization/access.hpp>
#include <boost/serialization/string.hpp>

namespace adservice {
namespace adselectv2 {

    class AdProductPackage {
    public:
        int ppid;
        std::string ast;

    public:
        AdProductPackage()
        {
        }
        AdProductPackage(const AdProductPackage & other)
        {
            copy(other);
        }

        void copy(const AdProductPackage & other)
        {
            ppid = other.ppid;
            ast = other.ast;
        }
        AdProductPackage & operator=(const AdProductPackage & other)
        {
            copy(other);
            return *this;
        }

    private:
        friend class boost::serialization::access;

        template <class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & ppid;
            ar & ast;
        }
    };

    class AdSolution {
    public:
        int solutionId;
        int advId;
        int priceType;
        int solutionStatus;
        double offerPrice;
        double ctr;
        std::string ast;
        std::string past;
        std::string ppids;
        std::string bids;
        std::string dealId{ "0" };
        double rate;
        int budgetShare;
        AdSolution()
            : solutionId(0)
        {
        }
        void copy(const AdSolution & other)
        {
            solutionId = other.solutionId;
            advId = other.advId;
            priceType = other.priceType;
            solutionStatus = other.solutionStatus;
            offerPrice = other.offerPrice;
            ctr = other.ctr;
            ast = other.ast;
            ppids = other.ppids;
            bids = other.bids;
            dealId = other.dealId;
            rate = other.rate;
            budgetShare = other.budgetShare;
        }
        AdSolution(const AdSolution & other)
        {
            copy(other);
        }
        AdSolution & operator=(const AdSolution & other)
        {
            copy(other);
            return *this;
        }
        bool isValid()
        {
            return solutionId > 0;
        }

    private:
        friend class boost::serialization::access;

        template <class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & solutionId;
            ar & advId;
            ar & priceType;
            ar & solutionStatus;
            ar & offerPrice;
            ar & ctr;
            ar & ppids;
            ar & bids;
            ar & dealId;
            ar & rate;
            ar & budgetShare;
        }
    };

    class AdBanner {
    public:
        int bannerId;
        int bannerStatus;
        int width;
        int height;
        int bannerType;
        double offerPrice;
        double ctr;
        std::string json;
        std::string adxAdvId;
        std::string adxIndustryType;
        AdBanner()
            : bannerId(0)
        {
        }
        void copy(const AdBanner & other)
        {
            bannerId = other.bannerId;
            bannerStatus = other.bannerStatus;
            width = other.width;
            height = other.height;
            bannerType = other.bannerType;
            offerPrice = other.offerPrice;
            ctr = other.ctr;
            json = other.json;
            adxAdvId = other.adxAdvId;
            adxIndustryType = other.adxIndustryType;
        }
        AdBanner(const AdBanner & other)
        {
            copy(other);
        }
        AdBanner & operator=(const AdBanner & other)
        {
            copy(other);
            return *this;
        }
        void parse(const as_record * record);
        bool isValid()
        {
            return bannerId > 0;
        }

    private:
        friend class boost::serialization::access;

        template <class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & bannerId;
            ar & bannerStatus;
            ar & width;
            ar & height;
            ar & bannerType;
            ar & offerPrice;
            ar & ctr;
            ar & json;
            ar & adxAdvId;
            ar & adxIndustryType;
        }
    };

    class AdAdplace {
    public:
        int adplaceId;
        int adxId;
        std::string adxPid{ "0" };
        int cid;
        int mid;
        int costPrice;
        int mediaType;
        double basePrice;
        int width{ 0 };
        int height{ 0 };
        AdAdplace()
            : adplaceId(0)
        {
        }
        AdAdplace(const AdAdplace & other)
        {
            copy(other);
        }
        void copy(const AdAdplace & other)
        {
            adplaceId = other.adplaceId;
            adxId = other.adxId;
            adxPid = other.adxPid;
            cid = other.cid;
            mid = other.mid;
            costPrice = other.costPrice;
            mediaType = other.mediaType;
            basePrice = other.basePrice;
            width = other.width;
            height = other.height;
        }
        AdAdplace & operator=(const AdAdplace & other)
        {
            copy(other);
            return *this;
        }
        void parse(const as_record * record);
        bool isValid()
        {
            return adplaceId > 0;
        }

    private:
        friend class boost::serialization::access;

        template <class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & adplaceId;
            ar & adxId;
            ar & adxPid;
            ar & cid;
            ar & mid;
            ar & costPrice;
            ar & mediaType;
            ar & basePrice;
            ar & width;
            ar & height;
        }
    };
}
}

#endif // ADDATA_H
