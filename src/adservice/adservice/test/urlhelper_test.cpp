#include "utility/utility.h"
#include <mtty/aerospike.h>
#include <mtty/constants.h>
#include <iostream>


using namespace adservice::utility;


bool inDebugSession = false;
void * debugSession = nullptr;
MT::common::Aerospike aerospikeClient;

void testCypherUrl(){
    url::URLHelper url1("click.mtty.com/c?o=6636&s=com.brianbaek.popstar&x=15&r=83ca1ad9d5d91907268bd91fb2fadbfd&d=8&e=10843&c=10326&f=http%3A%2F%2Fshow.mtty.com%2Fv%3Fl%3D%25%25CLICK_URL_PRE_ENC%25%25%26a%3D0086-110000-110000%26b%3D130%26c%3D10326%26d%3D8%26e%3D10843%26s%3Dcom.brianbaek.popstar%26o%3D6636%26x%3D15%26r%3D83ca1ad9d5d91907268bd91fb2fadbfd%26u%3DARBzdx2lQuhcEErllM1yCRypDlSHHQ%3D%3DKF8bl7GWclxKjVF3aNDy4TJ7%26tm%3D1476779974%26pt%3D5%26ep%3D10000%26of%3D2&h=000&a=0086-110000-110000&ep=10000&pt=5&b=130&url=http%3A%2F%2Fqmqj2.xy.com%2Fnew%2FVbANZh",false);
    std::cout<<"url1:"<<url1.toUrl()<<std::endl;
    std::string cipherUrl = url1.cipherUrl();
    std::cout<<"encoded url1:"<<cipherUrl<<std::endl;
    url::URLHelper url2(cipherUrl,true);
    std::cout<<"url2:"<<url2.toUrl()<<std::endl;
    std::cout<<"encoded url2:"<<url2.cipherUrl()<<std::endl;
}

void testUrlInterface(){
    url::URLHelper url1("http","192.168.2.31","20007","s");
    url1.add("a","whatdd");
    url1.add("b","qqq");
    url1.addMacro("p","${AUCTION_PRICE}");
    url1.addMacro("url","${IURL}");
    //std::cout<<"url1:"<<url1.toUrl()<<std::endl;
    //std::cout<<"encoded url1:"<<url1.cipherUrl()<<std::endl;
    url::URLHelper url2(url1.cipherUrl(),true);
    //std::cout<<"url2:"<<url2.toUrl()<<std::endl;
    //std::cout<<"encoded url2:"<<url2.cipherUrl()<<std::endl;
}

void encodeUrl(const std::string& uri){
    url::URLHelper url1(uri,false);
    std::cout<<url1.cipherUrl()<<std::endl;
}

void decodeUrl(const std::string& uri){
    url::URLHelper url1(uri,true);
    std::cout<<url1.toUrl()<<std::endl;
    auto& paramMap = url1.getParamMap();
    for(auto& iter:paramMap){
        std::cout<<iter.first<<":"<<iter.second<<std::endl;
    }
}

int main(int argc, char ** argv)
{
     if(argc==3){
         std::string op = argv[1];
         if(op == "encode"){
                encodeUrl(argv[2]);
         }else{
                decodeUrl(argv[2]);
         }
     }else{
        testCypherUrl();
     }
     return 0;
}
