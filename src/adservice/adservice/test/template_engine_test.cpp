#include "utility/utility.h"
#include <mtty/aerospike.h>
#include <iostream>
#include <boost/regex.hpp>


using namespace adservice::utility;


bool inDebugSession = false;
void * debugSession = nullptr;
MT::common::Aerospike aerospikeClient;


class PerformanceWatcher{
       public:
           PerformanceWatcher(const std::string& n):name(n){
              beginTimeMs = time::getCurrentTimeStampMs();
           }

           ~PerformanceWatcher(){
               int64_t endTime = time::getCurrentTimeStampMs();
               LOG_DEBUG<<name<<" time collapses:"<<(endTime-beginTimeMs)<<"ms";
           }

       private:
           int64_t beginTimeMs;
           std::string name;
       };


void testTemplateEngine(const std::string& templateFile,const std::string& shadowFile){
    templateengine::TemplateEngine te(templateFile);
    const auto& m = te.getVariables(shadowFile);
    {
        PerformanceWatcher pw("testTemplateEngine fast version");
        std::string out;
        for(int i=0;i<1000000;i++){
                te.bindFast(m,out);
        }
    }
    {
        PerformanceWatcher pw("testTemplateEngine normal version");
        for(int i=0;i<1000000;i++){
                te.bind(m);
        }
    }
}

void testRegexp(const std::string& regexpFile,const std::string& shadowFile){
    const std::string & regexpContent = file::loadFile(regexpFile);
    const std::string & shadowFileContent = file::loadFile(shadowFile);
    boost::regex regexpression(regexpContent);
    boost::smatch match;
    if (boost::regex_match(shadowFileContent, match, regexpression)) {
        std::cout<<match.size()<<std::endl;
        for(uint32_t i=0;i<match.size();i++){
            std::cout<<"match:"<<match[i].matched<<",s:"<<match[i]<<std::endl;
        }
    }
}

int main(int argc, char ** argv)
{
    if(argc>=4){
        std::string op=argv[1];
        if(op=="testEngine"){
                testTemplateEngine(argv[2],argv[3]);
        }else{
                testRegexp(argv[2],argv[3]);
        }
    }else{
        testTemplateEngine("./nginx.conf.template","./nginx.conf");
    }
     return 0;
}

