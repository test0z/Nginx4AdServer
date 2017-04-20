#include "utility/utility.h"
#include "protocol/360/360max_price.h"
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
    url::urlDecode_f(argv[1],decodedInput,buf);
    int result = max360_price_decode(decodedInput);//nex_price_decode(decodedInput);
    std::cout<<"result:"<<result<<std::endl;
    return 0;
}
