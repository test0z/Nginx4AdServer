#include "core_scenario_manager.h"
#include "logging.h"
#include <boost/algorithm/string.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/map.hpp>
#include <fstream>
#include <mtty/utils.h>
#include <sstream>
#include <utility/utility.h>

namespace adservice {
namespace server {

    std::atomic<bool> ScenarioManager::started{ false };
    ScenarioManagerPtr ScenarioManager::manager = nullptr;

    ScenarioDict ScenarioManager::loadScenarioData(const char * file)
    {
        ScenarioDict dict;
        std::fstream fs(file, std::ios_base::in);
        if (!fs.good()) {
            LOG_ERROR << " can't open json file:" << file;
            return dict;
        }
        do {
            std::string str;
            std::getline(fs, str, '\n');
            if (fs.eof()) {
                break;
            }
            std::vector<std::string> rule;
            boost::split(rule, str, boost::is_any_of(" "));
            if (rule.size() != 3) {
                LOG_ERROR << "wrong rule," << str;
            } else {
                uint32_t ipStart = uint32_t(adservice::utility::ip::ipStringToInt(rule[0]));
                uint32_t ipEnd = uint32_t(adservice::utility::ip::ipStringToInt(rule[1]));
                int scenario = std::stoi(rule[2]);
                uint32_t cidr = ipStart & ipEnd;
                LOG_DEBUG << rule[0] << "," << rule[1] << std::hex << "," << ipStart << "," << ipEnd
                          << ",cidr:" << std::hex << cidr;
                dict.insert({ cidr, scenario });
            }
        } while (!fs.eof());
        fs.close();
        return std::move(dict);
    }

    void ScenarioManager::init(const char * scenarioFile, const char * outFile)
    {
        std::ifstream binFile(outFile, std::ios_base::in | std::ios_base::binary);
        if (!binFile.good()) {
            LOG_INFO << "no scenario bin file exist,try to create";
            this->scenarioDict = loadScenarioData(scenarioFile);
            std::ofstream binOutFile(outFile, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
            if (binOutFile.good()) {
                boost::archive::binary_oarchive oa(binOutFile);
                oa << this->scenarioDict;
            }
            binOutFile.close();
        } else {
            boost::archive::binary_iarchive ia(binFile);
            ia >> this->scenarioDict;
        }
        if (scenarioDict.size() > 0) {
            cidrTable = new uint32_t[scenarioDict.size()];
            int32_t i = 0;
            for (auto & p : scenarioDict) {
                cidrTable[i++] = p.first;
            }
            std::sort(cidrTable, cidrTable + scenarioDict.size());
        }
    }

    namespace {

        /**
         * 找到最后一个小于等于key的元素
         * @brief bSearch
         * @param cidrTable
         * @param size
         * @param key
         * @return
         */
        int bSearch(uint32_t * cidrTable, int size, uint32_t key)
        {
            int l = 0, h = size - 1;
            while (l <= h) {
                int mid = l + ((h - l) >> 1);
                if (cidrTable[mid] > key) {
                    h = mid - 1;
                } else {
                    l = mid + 1;
                }
            }
            return h;
        }
    }

    int ScenarioManager::getScenario(const char * ip) const
    {
        uint32_t ipInt = uint32_t(adservice::utility::ip::ipStringToInt(ip));
        int pos = bSearch(cidrTable, scenarioDict.size(), ipInt);
        if (pos >= 0) {
            uint32_t network = cidrTable[pos];
            if ((ipInt & network) == network) {
                auto iter = scenarioDict.find(network);
                if (iter != scenarioDict.end()) {
                    return iter->second;
                }
            }
        }
        return SOLUTION_SCENARIO_OTHER;
    }
}
}
