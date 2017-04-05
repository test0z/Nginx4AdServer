#ifndef TEMPLATE_ENGINE_H
#define TEMPLATE_ENGINE_H

#include <string>
#include <unordered_map>
#include <vector>

namespace adservice {
namespace utility {

    namespace templateengine {

        struct TemplateParam {
            int pos;
            std::string param;
            TemplateParam(int p1, const std::string & p2)
                : pos(p1)
                , param(p2)
            {
            }
        };

        class TemplateEngine {
        public:
            TemplateEngine(const std::string & templateFile)
            {
                compile(templateFile);
            }

            std::string bind(const std::unordered_map<std::string, std::string> & paramMap);

            void bindFast(const std::unordered_map<std::string, std::string> & paramMap, std::string & output);

            std::unordered_map<std::string, std::string> getVariables(const std::string & shadowFile);

        private:
            void compile(const std::string & templateFile);

        private:
            std::vector<TemplateParam> params;
            std::string templateFileContent;
            std::string templateRegexp;
        };
    }
}
}

#endif // TEMPLATE_ENGINE_H
