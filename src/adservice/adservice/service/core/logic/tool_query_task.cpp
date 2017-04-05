#include "tool_query_task.h"
#include "utility/utility.h"
#include <cppcms/json.h>
#include <mtty/mtuser.h>

namespace adservice {
namespace corelogic {

    const std::string TOOL_PARAM_ACTIONTYPE = "action_type";

    const std::string TOOL_PARAM_ACTION = "action";

    const std::string TOOL_PARAM_AUTH = "auth";

    using namespace adservice::utility;

    namespace {

        typedef ActionHandler std::function<void(cppcms::json::value & paramMap, HttpResponse & resp)>;

        class ActionRouter {
        public:
            ActionRouter()
            {
                use("urltool", [](cppcms::json::value & m, HttpResponse & resp) {

                    std::string & action = m.get(TOOL_PARAM_ACTION, "");
                    std::string & uri = m.get("uri", "");
                    if (action == "encode") {
                        url::URLHelper url1(uri, false);
                        resp.set_body(url1.cipherUrl());
                    } else if (action == "decode") {
                        url::URLHelper url1(uri, true);
                        std::stringstream ss;
                        ss << url1.toUrl() << std::endl;
                        auto & mp = url1.getParamMap();
                        for (auto & iter : mp) {
                            ss << iter.first << ":" << iter.second << std::endl;
                        }
                        resp.set_body(ss.str());
                    } else {
                        throw std::string("action not support,action:") + action;
                    }
                });
                use("config", [](cppcms::json::value & m, HttpResponse & resp) {
                    std::string & action = m.get(TOOL_PARAM_ACTION, "");
                    std::string & configtype = m.get("configtype", "");
                    if (configtype.empty()) {
                        throw "configtype is empty";
                    }
                    if (action == "update") {
                    }
                });
                use("useridtool", [](cppcms::json::value & m, HttpResponse & resp) {
                    std::string & action = m.get(TOOL_PARAM_ACTION, "");
                    std::string & input = m.get("input", "");
                    if (action == "decode") {
                        MT::User::UserID clientUid(input, true);
                        resp.set_body(clientUid.text());
                    } else if (action == "encode") {
                        MT::User::UserID clientUid(input, false);
                        resp.set_body(clientUid.cipher());
                    } else if (action == "encodedevice") {
                        resp.set_body(cypher::md5_encode(stringtool::toupper(input)));
                    } else {
                        throw std::string("action not support,action:") + action;
                    }
                });
            }

            ActionRouter & use(const std::string & action, ActionHandler & handler)
            {
                handlers.insert({ action, handler });
                return *this;
            }

            void route(const std::string & action, ParamMap & m, HttpResponse & resp)
            {
                auto & iter = handlers.find(action);
                if (iter == handlers.end()) {
                    throw "action handler not found.";
                } else {
                    iter->second(m, resp);
                }
            }

        private:
            std::unordered_map<std::string, ActionHandler> handlers;
        };
    }

    static ActionRouter toolActionRouter;

    protocol::log::LogPhaseType HandleToolQueryTask::currentPhase()
    {
        return protocol::log::MAPPING;
    }

    int HandleToolQueryTask::expectedReqStatus()
    {
        return 200;
    }

    void HandleToolQueryTask::onError(std::exception & e, adservice::utility::HttpResponse & resp)
    {
        LOG_ERROR << "error occured in HandleMappingQueryTask:" << e.what();
    }

    void HandleToolQueryTask::customLogic(ParamMap & paramMap, protocol::log::LogItem & log,
                                          adservice::utility::HttpResponse & resp)
    {
        cppcms::json::value requestData;
        json::parseJson(data.c_str(), requestData);
        if (!requestData.is_null() && !requestData.is_undefined()) {
            std::string & actionType = requestData.get(TOOL_PARAM_ACTIONTYPE, "");
            std::string & auth = requestData.get(TOOL_PARAM_AUTH, "");
            if (auth.empty()
                || md5_encode(stringtool::toupper(auth)) != md5_encode(stringtool::toupper("zhimakaimen"))) {
                resp.status(403, "forbidden");
                resp.set_body("wrong way to go dude ^_^");
            }
            if (!actionType.empty()) {
                try {
                    toolActionRoute.exec(actionType, paramMap, resp);
                    return;
                } catch (const std::string & errStr) {
                    resp.status(400, "query param invalid");
                    resp.set_body(errStr);
                    return;
                } catch (...) {
                }
            }
        }
        resp.status(400, "query param invalid");
        resp.set_body("query param invalid");
    }
}
}
