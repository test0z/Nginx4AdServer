#ifndef CORE_AD_SIZEMAP_H
#define CORE_AD_SIZEMAP_H

#include <map>

namespace adservice {
namespace utility {

    class AdSizeMap {
    public:
        static const AdSizeMap & getInstance();

    public:
        AdSizeMap()
        {
            add(std::make_pair(270, 202), std::make_pair(1, 0));
            add(std::make_pair(240, 180), std::make_pair(1, 1));
            add(std::make_pair(984, 328), std::make_pair(2, 0));
            add(std::make_pair(1200, 800), std::make_pair(2, 1));
            add(std::make_pair(1280, 720), std::make_pair(2, 2));
            add(std::make_pair(1200, 627), std::make_pair(2, 3));
            add(std::make_pair(800, 1200), std::make_pair(2, 4));
            add(std::make_pair(640, 288), std::make_pair(2, 5));
            add(std::make_pair(1000, 560), std::make_pair(2, 6));
            add(std::make_pair(270, 202), std::make_pair(3, 0));
            add(std::make_pair(480, 240), std::make_pair(2, 7));
            add(std::make_pair(640, 320), std::make_pair(2, 8));
            add(std::make_pair(1080, 540), std::make_pair(2, 9));
            add(std::make_pair(360, 234), std::make_pair(1, 2));
            add(std::make_pair(656, 324), std::make_pair(2, 10));
            add(std::make_pair(228, 162), std::make_pair(1, 3));
            add(std::make_pair(750, 350), std::make_pair(2, 11));
            add(std::make_pair(900, 450), std::make_pair(2, 12));
        }
        void add(const std::pair<int, int> & k, const std::pair<int, int> & v)
        {
            sizemap.insert(std::make_pair(k, v));
            rsizemap.insert(std::make_pair(v, k));
        }

        std::pair<int, int> get(const std::pair<int, int> & k) const
        {
            auto iter = sizemap.find(k);
            if (iter != sizemap.end()) {
                return iter->second;
            } else {
                return k;
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
        std::map<std::pair<int, int>, std::pair<int, int>> sizemap;
        std::map<std::pair<int, int>, std::pair<int, int>> rsizemap;

    private:
        static AdSizeMap instance_;
    };
}
}

#endif // CORE_AD_SIZEMAP_H
