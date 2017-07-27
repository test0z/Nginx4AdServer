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

        // todo:实现一个支持动态脚本语言的vm,模板引擎可以在服务端渲染时调用vm执行脚本，最终效果类似php
        //虚拟机运行时
        class RunTime;

        //运行时类型
        enum class RunTimeTypeEnum { BOOLEAN, NUMBER, STRING, FUNCTION };

        //表达式中的符号类型
        enum class ExpressionSymbolType { CONST_BOOLEAN, CONST_NUMBER, CONST_STRING, FUNCTION, VARIABLE };

        class ExpressionSymbol {
        public:
            ExpressionSymbolType getType()
            {
                return type;
            }

        private:
            ExpressionSymbolType type;
        };

        class Expression {
        public:
            explicit Expression(const std::string & expr);

            std::string eval(const RunTime & vm);

        private:
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
