#include "core_ad_sizemap.h"

namespace adservice {
namespace utility {

    AdSizeMap AdSizeMap::instance_;

    const AdSizeMap & AdSizeMap::getInstance()
    {
        return instance_;
    }

    AdSizeMap::AdSizeMap()
    {
        sizemap[MATERIAL_TYPE_PIC] = std::map<std::pair<int, int>, std::vector<std::pair<int, int>>>();
        sizemap[MATERIAL_TYPE_VIDEO] = std::map<std::pair<int, int>, std::vector<std::pair<int, int>>>();
        add(std::make_pair(270, 202), std::make_pair(1, 0));
        add(std::make_pair(240, 180), std::make_pair(1, 1));
        add(std::make_pair(360, 234), std::make_pair(1, 2));
        add(std::make_pair(228, 162), std::make_pair(1, 3));
        add(std::make_pair(180, 92), std::make_pair(1, 4));
        add(std::make_pair(320, 150), std::make_pair(1, 5));
        add(std::make_pair(200, 100), std::make_pair(1, 6));
        add(std::make_pair(150, 200), std::make_pair(1, 7));
        add(std::make_pair(200, 200), std::make_pair(1, 8));
        add(std::make_pair(72, 72), std::make_pair(1, 9));
        add(std::make_pair(160, 160), std::make_pair(1, 10));
        add(std::make_pair(200, 150), std::make_pair(1, 11));
        add(std::make_pair(180, 100), std::make_pair(1, 12));
        add(std::make_pair(140, 88), std::make_pair(1, 13));
        add(std::make_pair(984, 328), std::make_pair(2, 0));
        add(std::make_pair(1200, 800), std::make_pair(2, 1));
        add(std::make_pair(1280, 720), std::make_pair(2, 2));
        add(std::make_pair(1200, 627), std::make_pair(2, 3));
        add(std::make_pair(800, 1200), std::make_pair(2, 4));
        add(std::make_pair(640, 288), std::make_pair(2, 5));
        add(std::make_pair(1000, 560), std::make_pair(2, 6));
        add(std::make_pair(480, 240), std::make_pair(2, 7));
        add(std::make_pair(640, 320), std::make_pair(2, 8));
        add(std::make_pair(1080, 540), std::make_pair(2, 9));
        add(std::make_pair(656, 324), std::make_pair(2, 10));
        add(std::make_pair(750, 350), std::make_pair(2, 11));
        add(std::make_pair(900, 450), std::make_pair(2, 12));
        add(std::make_pair(600, 313), std::make_pair(2, 13));
        add(std::make_pair(300, 250), std::make_pair(2, 14));
        add(std::make_pair(320, 568), std::make_pair(2, 15));
        add(std::make_pair(720, 240), std::make_pair(2, 16));
        add(std::make_pair(640, 290), std::make_pair(2, 17));
        add(std::make_pair(200, 700), std::make_pair(2, 18));
        add(std::make_pair(720, 405), std::make_pair(2, 19));
        add(std::make_pair(720, 360), std::make_pair(2, 20));
        add(std::make_pair(720, 480), std::make_pair(2, 21));
        add(std::make_pair(660, 220), std::make_pair(2, 22));
        add(std::make_pair(270, 202), std::make_pair(3, 0));
        add(std::make_pair(160, 160), std::make_pair(3, 1));
        add(std::make_pair(984, 328), std::make_pair(4, 0), MATERIAL_TYPE_VIDEO);
    }
}
}
