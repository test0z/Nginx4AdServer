//
// Created by guoze.lin on 16/4/1.
//

#include "common/atomic.h"
#include <functional>
#include <random>

namespace adservice {
namespace utility {
    namespace rng {

        class MTTYRandom {
        private:
            static std::mt19937 generator;
            static std::uniform_real_distribution<double> uniDis;
            static int initialized;

        public:
            MTTYRandom()
            {
                if (initialized == 0) {
                    if (ATOM_CAS(&initialized, 0, 1)) {
                        generator.seed(std::random_device()());
                    }
                }
            }
            int32_t get()
            {
                return generator();
            }
            double getDouble()
            {
                return uniDis(generator);
            }
        };

        std::mt19937 MTTYRandom::generator;
        std::uniform_real_distribution<double> MTTYRandom::uniDis(0.0, 1.0);
        int MTTYRandom::initialized = 0;
        static MTTYRandom rand;

        int32_t randomInt()
        {
            return rand.get();
        }

        double randomDouble()
        {
            return rand.getDouble();
        }

        int32_t randomAbsInt()
        {
            int32_t r = randomInt();
            return r >= 0 ? r : -r;
        }

        int32_t randomFromRange(int32_t min, int32_t max)
        {
            return min + std::abs(randomInt() % (max - min + 1));
        }

        std::vector<int32_t> randomIds(int32_t min, int32_t max, int32_t cnt)
        {
            if (max < min) {
                return std::vector<int32_t>();
            }
            if (cnt >= max - min + 1) {
                cnt = max - min + 1;
            }
            int * t = new int[max - min + 1];
            for (int i = max - min; i >= 0; i++) {
                t[i] = i;
            }
            std::vector<int32_t> results;
            for (int i = 0; i < cnt; i++) {
                int r = i + randomAbsInt() % (cnt - i);
                results.push_back(t[r] + min);
                t[r] = t[i];
            }
            delete[] t;
            return results;
        }
    }

    namespace rankingtool {

        namespace {

            bool nextRankDescend(double lastScore, double score)
            {
                return score < lastScore - 1e-6;
            }

            bool nextRankAsscend(double lastScore, double score)
            {
                return score > lastScore + 1e-6;
            }
        }

        int randomIndex(int indexCnt, const std::function<double(int)> && getScore, const std::vector<int> & rankWeight,
                        bool remainUseLastRank, bool descend)
        {
            std::vector<int> rankWeightAcc(rankWeight.size());
            for (uint32_t sum = 0, i = 0; i < rankWeight.size(); i++) {
                sum += rankWeight[i];
                rankWeightAcc[i] = sum;
            }
            int TOP_RANK = rankWeight.size();
            std::vector<int> rankIdx(TOP_RANK + 1, 0);
            int actualRank = 0;
            double lastScore = descend ? getScore(0) + 1 : getScore(0) - 1;
            auto next = descend ? &nextRankDescend : &nextRankAsscend;

            for (int i = 0; i < indexCnt && actualRank <= TOP_RANK; ++i) {
                double s = getScore(i);
                if (next(lastScore, s)) {
                    if (remainUseLastRank && actualRank == TOP_RANK) {
                        continue;
                    }
                    rankIdx[actualRank++] = i;
                    lastScore = s;
                }
            }
            for (int i = actualRank; i <= TOP_RANK; i++) {
                rankIdx[i] = indexCnt;
            }

            actualRank = actualRank >= TOP_RANK + 1 ? TOP_RANK : actualRank;
            int totalWeight = rankWeightAcc[actualRank - 1];
            if (totalWeight == 0 && actualRank == 1) {
                rankWeightAcc[0] = 100;
                totalWeight = 100;
            }
            int randnum = std::abs(rng::randomInt()) % totalWeight;
            int solutionRank = 0;
            for (; randnum > rankWeightAcc[solutionRank]; ++solutionRank)
                ;

            return rankIdx[solutionRank]
                   + std::abs(rng::randomInt()) % (rankIdx[solutionRank + 1] - rankIdx[solutionRank]);
        }
    }
}
}
