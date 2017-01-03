#include "utility/utility.h"
#include "protocol/netease/nex_price.h"
#include <mtty/constants.h>
#include <mtty/aerospike.h>
#include <iostream>
#include <string>


using namespace adservice::utility;

bool inDebugSession = false;
void * debugSession = nullptr;
MT::common::Aerospike aerospikeClient;

int main(int argc,char** argv){
    char buf[1024];
    std::string decodedInput;
    url::urlDecode_f("Nk0f6s7jza5h9kIMtiTtbA%3D%3D",decodedInput,buf);
    int result = nex_price_decode(decodedInput);
    std::cout<<"result:"<<result<<std::endl;
    return 0;
}
