//
// Created by guoze.lin on 16/4/1.
//

#include <random>
#include "common/atomic.h"

namespace adservice{
    namespace utility{
        namespace rng{

            class MTTYRandom{
            private:
                static std::mt19937 generator;
                static std::uniform_real_distribution<double> uniDis;
                static int initialized;
            public:
                MTTYRandom(){
                    if(initialized==0){
                        if(ATOM_CAS(&initialized,0,1)){
                            generator.seed(std::random_device()());
                        }
                    }
                }
                int32_t get(){
                    return generator();
                }
                double getDouble(){
                    return uniDis(generator);
                }
            };

            std::mt19937 MTTYRandom::generator;
            std::uniform_real_distribution<double> MTTYRandom::uniDis(0.0,1.0);
            int MTTYRandom::initialized = 0;
            static MTTYRandom rand;

            int32_t randomInt(){
                return rand.get();
            }

            double randomDouble(){
                return rand.getDouble();
            }

        }
    }
}
