#ifndef CORE_AD_SIZEMAP_H
#define CORE_AD_SIZEMAP_H

#include <map>
#include <vector>

namespace adservice {
namespace utility {

    class AdSizeMap {
    public:
        static const AdSizeMap & getInstance();
#define MATERIAL_TYPE_PIC 0
#define MATERIAL_TYPE_VIDEO 1

    public:
        AdSizeMap();

        void add(const std::pair<int, int> & k, const std::pair<int, int> & v, int type = MATERIAL_TYPE_PIC)
        {

            auto iter = sizemap[type].find(k);
            if (iter != sizemap[type].end()) {
                iter->second.push_back(v);
            } else {
                sizemap[type].insert({ k, std::vector<std::pair<int, int>>{ v } });
            }
            rsizemap.insert(std::make_pair(v, k));
        }

        std::vector<std::pair<int, int>> get(const std::pair<int, int> & k, int type = MATERIAL_TYPE_PIC) const
        {
            auto it = sizemap.find(type);
            if (it == sizemap.end()) {
                return std::vector<std::pair<int, int>>{ k };
            }
            auto & typeSizeMap = it->second;
            auto iter = typeSizeMap.find(k);
            if (iter != typeSizeMap.end()) {
                return iter->second;
            } else {
                return std::vector<std::pair<int, int>>{ k };
            }
        }

        std::pair<int, int> rget(const std::pair<int, int> & k) const
        {
            auto iter = rsizemap.find(k);
            if (iter != rsizemap.end()) {
                return iter->second;
            } else {
                return k;
            }
        }

    private:
        void initFromDB();

        void initStatic();

        static int started;

    private:
        std::map<int, std::map<std::pair<int, int>, std::vector<std::pair<int, int>>>> sizemap;
        std::map<std::pair<int, int>, std::pair<int, int>> rsizemap;

    private:
        static AdSizeMap instance_;
    };
}
}

#endif // CORE_AD_SIZEMAP_H
