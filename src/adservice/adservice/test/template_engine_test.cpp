#include "utility/utility.h"
#include <mtty/aerospike.h>
#include <iostream>
#include <boost/regex.hpp>


using namespace adservice::utility;


bool inDebugSession = false;
void * debugSession = nullptr;
MT::common::Aerospike aerospikeClient;


void testTemplateEngine(const std::string& templateFile,const std::string& shadowFile){
    templateengine::TemplateEngine te(templateFile);
    const auto& m = te.getVariables(shadowFile);
    for(auto& iter:m){
        std::cout<<iter.first<<":"<<iter.second<<",";
    }
    std::cout<<"template replace:"<<endl;
    std::cout<<te.bind(m);
}

void testRegexp(const std::string shadowFile){
    const std::string & shadowFileContent = file::loadFile(shadowFile);
    boost::regex regexpression("worker_processes  (.*);");
    boost::smatch match;
    if (boost::regex_search(shadowFileContent, match, regexpression)) {
        std::cout<<match.size()<<std::endl;
        for(uint32_t i=0;i<match.size();i++){
            std::cout<<"match:"<<match[i].matched<<",s:"<<match[i]<<std::endl;
        }
    }
}

int main(int argc, char ** argv)
{
    if(argc>2){
        testTemplateEngine(argv[1],argv[2]);
    }
     return 0;
}
