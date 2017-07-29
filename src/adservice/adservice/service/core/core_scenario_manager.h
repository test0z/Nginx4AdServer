#ifndef CORE_SCENARIO_MANAGER_H
#define CORE_SCENARIO_MANAGER_H

#include <atomic>
#include <map>
#include <memory>
#include <mtty/constants.h>
#include <string>

namespace adservice {
namespace server {

    typedef std::map<uint32_t, int> ScenarioDict;

    class ScenarioManager;
    typedef std::shared_ptr<ScenarioManager> ScenarioManagerPtr;

    class ScenarioManager {
    public:
        void init(const char * scenarioFile = "res/scenario.txt", const char * outFile = "res/scenario.bin");

        ~ScenarioManager()
        {
            if (cidrTable) {
                delete[] cidrTable;
                cidrTable = nullptr;
            }
        }

        static ScenarioDict loadScenarioData(const char * areafile);
        static ScenarioManagerPtr & getInstance()
        {
            if (started.exchange(true) == false) {
                manager = std::make_shared<ScenarioManager>();
                manager->init();
            }
            return manager;
        }
        static std::atomic<bool> started;
        static ScenarioManagerPtr manager;

    private:
        ScenarioDict scenarioDict;
        uint32_t * cidrTable{ nullptr };

    public:
        ScenarioManager()
        {
        }
        ScenarioManager(const ScenarioManager &) = delete;
        int getScenario(const char * ip) const;
    };
}
}

#endif // CORE_SCENARIO_MANAGER_H
