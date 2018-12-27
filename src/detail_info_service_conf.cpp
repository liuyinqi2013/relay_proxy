
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include "trpc_sys_exception.h"
#include "trpc_comm_func.h"
//#include "service_stats_conf.h"

#include "detail_info_service_conf.h"

DetailInfoServiceConf::DetailInfoServiceConf()
{
}

void
DetailInfoServiceConf::read_conf(const char *config_file)
{
    // ?車D∩??
    CAutoWLock  l(_mutex);
    
    detail_info_service_set_.clear();

    FILE *fp = fopen(config_file, "r");

    if (fp == NULL)
    {
        THROW(CConfigException, ERR_CONF, "error code escape conf error, invalid config file:%s", config_file);
    }

    char buffer[512];
    std::vector<std::string> str_vec;
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        strstrip(buffer);

        if (strlen(buffer) <= 1) continue;
        if (buffer[0] == '#')    continue;

        detail_info_service_set_.insert(atoi(buffer));
    }

    fclose(fp);
}

bool
DetailInfoServiceConf::is_detail_info_service(int request_type)
{
    // ?車?芍??
    CAutoRLock  l(_mutex);
    
    return detail_info_service_set_.count(request_type) > 0;
}

void 
DetailInfoServiceConf::reload(const char * conf_flie)
{
    // ?Y那㊣?㊣?車米¯車?read_confo‘那y
    read_conf(conf_flie);
}

