//
// Created by guoze.lin on 16/6/6.
//

#include "core_ip_manager.h"
#include "common/atomic.h"
#include "common/functions.h"
#include "logging.h"
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char byte;
typedef unsigned int uint;
#define B2IL(b) \
    (((b)[0] & 0xFF) | (((b)[1] << 8) & 0xFF00) | (((b)[2] << 16) & 0xFF0000) | (((b)[3] << 24) & 0xFF000000))
#define B2IU(b) \
    (((b)[3] & 0xFF) | (((b)[2] << 8) & 0xFF00) | (((b)[1] << 16) & 0xFF0000) | (((b)[0] << 24) & 0xFF000000))

struct {
    byte * data;
    byte * index;
    uint * flag;
    uint offset;
} ipip;

int ipDataDestroy()
{
    if (!ipip.offset) {
        return 0;
    }
    free(ipip.flag);
    free(ipip.index);
    free(ipip.data);
    ipip.offset = 0;
    return 0;
}

int ipDataInit(const char * ipdb)
{
    if (ipip.offset) {
        LOG_ERROR << "offset failed";
        return -1;
    }
    FILE * file = fopen(ipdb, "rb");
    if (!file) {
        LOG_ERROR << "failed to openFile";
        return -1;
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    ipip.data = (byte *)malloc(size * sizeof(byte));
    size_t r = fread(ipip.data, sizeof(byte), (size_t)size, file);

    if (r == 0) {
        return -1;
    }

    fclose(file);

    uint length = B2IU(ipip.data);

    ipip.index = (byte *)malloc(length * sizeof(byte));
    memcpy(ipip.index, ipip.data + 4, length);

    ipip.offset = length;

    ipip.flag = (uint *)malloc(256 * sizeof(uint));
    memcpy(ipip.flag, ipip.index, 256 * sizeof(uint));

    return 0;
}

int ipDataFind(const char * ip, char * result)
{
    uint ips[4];
    int num = sscanf(ip, "%d.%d.%d.%d", &ips[0], &ips[1], &ips[2], &ips[3]);
    if (num == 4) {
        uint ip_prefix_value = ips[0];
        uint ip2long_value = B2IU(ips);
        uint start = ipip.flag[ip_prefix_value];
        uint max_comp_len = ipip.offset - 1028;
        uint index_offset = 0;
        uint index_length = 0;
        for (start = start * 8 + 1024; start < max_comp_len; start += 8) {
            if (B2IU(ipip.index + start) >= ip2long_value) {
                index_offset = B2IL(ipip.index + start + 4) & 0x00FFFFFF;
                index_length = ipip.index[start + 7];
                break;
            }
        }
        memcpy(result, ipip.data + ipip.offset + index_offset - 1024, index_length);
        result[index_length] = '\0';
        return index_length;
    }
    return 0;
}

namespace adservice {
namespace server {

    AreaDict IpManager::areaDict;
    int IpManager::started = 0;

    void processAreaFileLine(const char * line, std::string & name, int & country, int & province, int & city)
    {
        const char * p = line;
        while (*p != '\t' && *p != ' ')
            p++;
        name = std::string(line, p);
        p++;
        country = atoi(p);
        while (*p != '\n' && *p != '-')
            p++;
        if (*p == '\n')
            return;
        p++;
        province = atoi(p);
        while (*p != '\n' && *p != '-')
            p++;
        if (*p == '\n')
            return;
        p++;
        city = atoi(p);
    }

    void IpManager::loadAreaData(const char * areafile)
    {
        FILE * file = fopen(areafile, "r");
        if (!file) {
            LOG_ERROR << "IpManager::loadAreaData areafile can not be opened! file:" << areafile;
            return;
        }
        char buffer[256];
        std::map<int, AreaItem> countryMap;
        std::map<int, AreaItem> provinceMap;
        std::map<int, AreaItem> cityMap;
        while (fgets(buffer, sizeof(buffer), file) != NULL) {
            std::string name;
            int country = 0, province = 0, city = 0;
            processAreaFileLine(buffer, name, country, province, city);
            if (country != 0 && province == 0) { // country
                AreaItem areaItem(name, country, province, city);
                countryMap.insert(std::make_pair(country, areaItem));
            } else if (country != 0 && province != 0 && city == 0) { // province
                std::string countryName = countryMap[country].name;
                AreaItem areaItem(countryName + name, country, province, city);
                provinceMap.insert(std::make_pair(country * AREACODE_MARGIN + province, areaItem));
            } else if (country != 0 && province != 0 && city != 0) { // city
                int provinceCode = country * AREACODE_MARGIN + province;
                std::string provinceName = provinceMap[provinceCode].name;
                AreaItem areaItem(provinceName + name, country, province, city);
                cityMap.insert(std::make_pair(country * AREACODE_MARGIN + city, areaItem));
            }
        }
        for (auto iter = countryMap.begin(); iter != countryMap.end(); iter++) {
            AreaDictAccessor acc;
            areaDict.insert(acc, iter->second.name);
            acc->second = iter->second;
            acc.release();
        }
        for (auto iter = provinceMap.begin(); iter != provinceMap.end(); iter++) {
            AreaDictAccessor acc;
            areaDict.insert(acc, iter->second.name);
            acc->second = iter->second;
            acc.release();
        }
        for (auto iter = cityMap.begin(); iter != cityMap.end(); iter++) {
            AreaDictAccessor acc;
            areaDict.insert(acc, iter->second.name);
            acc->second = iter->second;
            acc.release();
        }
        fclose(file);
    }

    void IpManager::init(const char * ipdataFile, const char * areaCodeFile)
    {
        if (!ATOM_CAS(&started, 0, 1)) {
            return;
        }
        if (ipDataInit(ipdataFile) < 0) {
            LOG_ERROR << "init ipdata from data file failed,file:" << ipdataFile;
            return;
        }
        loadAreaData(areaCodeFile);
    }

    void IpManager::destroy()
    {
        if (!ATOM_CAS(&started, 1, 0)) {
            return;
        }
        ipDataDestroy();
    }

    int getAreaShortCode(const AreaItem & areaItem)
    {
        if (areaItem.province == 0) {
            return areaItem.country * AREACODE_MARGIN;
        } else if (areaItem.city == 0) {
            return areaItem.country * AREACODE_MARGIN + areaItem.province;
        } else {
            return areaItem.country * AREACODE_MARGIN + areaItem.city;
        }
    }

    int trim(char * buffer)
    {
        char *p1 = buffer, *p2 = p1;
        while (*p1 != '\0') {
            while (*p1 != ' ' && *p1 != '\t' && *p1 != '\0') {
                *p2++ = *p1++;
            }
            if (*p1 != '\0') {
                while (*p1 == ' ' || *p1 == '\t')
                    p1++;
            }
        }
        *p2 = '\0';
        return p2 - buffer;
    }

    int IpManager::getAreaByIp(const char * ip)
    {
        char name[256];
        int length = ipDataFind(ip, name);
        length = trim(name);
        std::string areaName = std::string(name, name + length);
        int code = 0;
        AreaDictAccessor acc;
        if (areaDict.find(acc, areaName)) {
            AreaItem areaItem = acc->second;
            code = getAreaShortCode(areaItem);
        } else {
            code = IP_UNKNOWN;
        }
        acc.release();
        return code;
    }

    std::string IpManager::getAreaNameByIp(const char * ip)
    {
        char name[256];
        int length = ipDataFind(ip, name);
        length = trim(name);
        std::string areaName = std::string(name, name + length);
        return areaName;
    }

    std::string IpManager::getAreaCodeStrByIp(const char * ip)
    {
        char name[256];
        int length = ipDataFind(ip, name);
        length = trim(name);
        std::string areaName = std::string(name, name + length);
        char code[64] = { '\0' };
        AreaDictAccessor acc;
        if (areaDict.find(acc, areaName)) {
            AreaItem areaItem = acc->second;
            snprintf(code, sizeof(code), "%04d-%d-%d", areaItem.country, areaItem.province, areaItem.city);
        } else {
            strcat(code, "0086-0-0");
        }
        acc.release();
        return std::string(code);
    }

    void IpManager::getAreaCodeByIp(const char * ip, int & country, int & province, int & city)
    {
        char name[256];
        int length = ipDataFind(ip, name);
        length = trim(name);
        std::string areaName = std::string(name, name + length);
        AreaDictAccessor acc;
        if (areaDict.find(acc, areaName)) {
            AreaItem areaItem = acc->second;
            country = areaItem.country;
            province = areaItem.province;
            city = areaItem.city;
        } else {
            country = 86;
            province = 0;
            city = 0;
        }
        acc.release();
    }
}
}
