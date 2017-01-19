
#include "utility/utility.h"
#include <mtty/constants.h>
#include <mtty/aerospike.h>
#include "core/core_executor.h"
#include <iostream>
#include <string>


using namespace adservice::utility;
using namespace adservice::server;

bool inDebugSession = false;
void * debugSession = nullptr;
MT::common::Aerospike aerospikeClient;

int main(int argc,char** argv){
    Executor executor("test",false,10,5000);
    executor.start();
    int count = 0;
    while(true){
        int rand = std::abs(rng::randomInt())%100;
        for(int i=0;i<rand;i++){
            executor.run([](){
                for(int i=0;i<1000;i++){
                    cppcms::json::value doc;
                    json::parseJson("{\"abc\":true}",doc);
                }
            });
        }
        count+=rand;
        if(count%10 == 1){
            std::cout<<"run tasks:"<<count<<",pending:"<<executor.pendingTasks()<<std::endl;
        }
        if(count>100000){
            break;
        }
    }
    while(executor.pendingTasks()>0){
        std::cout<<"waiting to leave..."<<std::endl;
        sleep(1);
    }
    executor.stop();
    return 0;
}
