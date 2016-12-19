#include "utility/utility.h"
#include <mtty/aerospike.h>
#include <mtty/constants.h>
#include <iostream>


using namespace adservice::utility;


bool inDebugSession = false;
void * debugSession = nullptr;
MT::common::Aerospike aerospikeClient;

void testCypherUrl(){
    url::URLHelper url1("http://192.168.2.31:20007/asdfasdf?a=saffd&b=wqrasfd&q=124312",false);
    url1.add("qi","what");
    url1.addMacro("p","${AUCTION_PRICE}");
    url1.addMacro("b2","${BID_PRICE}");
    std::cout<<"url1:"<<url1.toUrl()<<std::endl;
    std::cout<<"encoded url1:"<<url1.cipherUrl()<<std::endl;
    url::URLHelper url2(url1.cipherUrl(),true);
    std::cout<<"url2:"<<url2.toUrl()<<std::endl;
    std::cout<<"encoded url2:"<<url2.cipherUrl()<<std::endl;
}

void testUrlInterface(){
    url::URLHelper url1("http","192.168.2.31","20007","s");
    url1.add("a","whatdd");
    url1.add("b","qqq");
    url1.addMacro("p","${AUCTION_PRICE}");
    url1.addMacro("url","${IURL}");
    std::cout<<"url1:"<<url1.toUrl()<<std::endl;
    std::cout<<"encoded url1:"<<url1.cipherUrl()<<std::endl;
    url::URLHelper url2(url1.cipherUrl(),true);
    std::cout<<"url2:"<<url2.toUrl()<<std::endl;
    std::cout<<"encoded url2:"<<url2.cipherUrl()<<std::endl;
}

int main(int argc, char ** argv)
{
    //testCypherUrl();
    testUrlInterface();
    return 0;
}
