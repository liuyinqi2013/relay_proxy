#ifndef __SWITCH_STATS_CONF_H__
#define __SWITCH_STATS_CONF_H__

#include <vector>
#include <map>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <sstream>

#include "trpc_sys_exception.h"
#include "trpc_comm_func.h"

struct stats_conf
{
    std::string key_id;
    int  time_interval;
    int  minimal_success_ratio;
    int  minimal_request_threshold;
    std::vector<int>  errcode_list;
};

template<typename T>
class SwitchStatsConf
{
public:
    SwitchStatsConf();
    ~SwitchStatsConf();

    bool isExists(const T& switch_key, int error_code);

    void  init(const char *conf_file);
    void  re_init(const char *conf_file);

private:
    void read_conf(const char *conf_file);
    

//private:
public:
    typedef std::map<T, struct stats_conf> STATS_CONF_MAP;
    std::map<T, struct stats_conf>   switch_stats_info;

    CRwLock          mutex_;
    bool             is_init_;
};

typedef SwitchStatsConf<std::string> ServiceStatsConf;
typedef SwitchStatsConf<int>         SiteStatsConf;


template<typename T>
SwitchStatsConf<T>::SwitchStatsConf()
:is_init_(false)
{
}

template<typename T>
SwitchStatsConf<T>::~SwitchStatsConf()
{
}

template<typename T>
void
SwitchStatsConf<T>::init(const char *conf_file)
{
    CAutoWLock l(mutex_);

    read_conf(conf_file);

    is_init_ = true;
}

template<typename T>
void
SwitchStatsConf<T>::re_init(const char *conf_file)
{
    CAutoWLock l(mutex_);

    is_init_ = false;

    switch_stats_info.clear();

    read_conf(conf_file);

    is_init_ = true;
}

template<typename T>
void
SwitchStatsConf<T>::read_conf(const char *conf_file)
{
    FILE *fp = fopen(conf_file, "r");

    if (fp == NULL)
    {
        THROW(CConfigException, ERR_CONF, "error code escape conf error, invalid config file:%s", conf_file);
    }

    //static char filter_chars[] = { 10, 13, 32 };

    char buffer[2048];
    std::vector<std::string> str_vec;
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        //trimString(buffer, filter_chars);
        strstrip(buffer);
        if (strlen(buffer) <= 1) continue;
        if (buffer[0] == '#')    continue;

        split(buffer, "|", 8, str_vec);

        assert(str_vec.size() >= 5);

        struct stats_conf conf_item;
        conf_item.key_id = str_vec[1];
        conf_item.time_interval = atoi(str_vec[2].c_str());
        conf_item.minimal_success_ratio = atoi(str_vec[3].c_str());
        conf_item.minimal_request_threshold = atoi(str_vec[4].c_str());

        if (str_vec.size() > 5)
        {
            std::vector<std::string> error_vec;
            split(str_vec[5].c_str(), ",", 1000, error_vec);
            for(std::vector<std::string>::const_iterator iter = error_vec.begin(); iter != error_vec.end(); ++iter)
            {
                conf_item.errcode_list.push_back(atoi((*iter).c_str()));
            }
            std::sort(conf_item.errcode_list.begin(), conf_item.errcode_list.end());
        }
        else
        {
            conf_item.errcode_list.clear();
        }

        T key = type_cast<std::string, T>(str_vec[0]);
        switch_stats_info[key] = conf_item;
    }

    fclose(fp);

    //for test
    /*
    for(typename std::map<T, struct stats_conf>::const_iterator i = switch_stats_info.begin();
        i != switch_stats_info.end();
        ++i)
    {
        std::string key_str = type_cast<T, std::string>((*i).first);
        printf("key:%s\n", key_str.c_str());
        printf("    time_interval:%d\n", (*i).second.time_interval);
        printf("    minimal_success_ratio:%d\n", (*i).second.minimal_success_ratio);
        printf("    minimal_request_threshold:%d\n", (*i).second.minimal_request_threshold);
        const std::vector<int> & error_list = (*i).second.errcode_list;
        printf("    error list:");
        for(std::vector<int>::const_iterator iter = error_list.begin(); iter != error_list.end(); ++iter)
        {
            printf("%d,", (*iter));
        }
        printf("\n");
    }
    */
}

/*
template<typename T>
bool
SwitchStatsConf<T>::is_key_service(const T& switch_key)
{
    if (is_init_)
    {
        CAutoRLock l(mutex_);
        if (is_init_)
        {
            return switch_stats_info.find(switch_key) != switch_stats_info.end();
        }
    }
}

template<typename T>
int  
SwitchStatsConf<T>::time_interval(const T& switch_key)
{
    if (is_init_)
    {
        CAutoRLock l(mutex_);
        if (is_init_)
        {
            return switch_stats_info[switch_key].time_interval;
        }
    }
}

template<typename T>
int  
SwitchStatsConf<T>:: minimal_succ_ratio(const T& switch_key)
{
    return switch_stats_info[switch_key].minimal_success_ratio;
}

template<typename T>
int
SwitchStatsConf<T>::request_threshold(const T& switch_key)
{
    return switch_stats_info[switch_key].minimal_request_threshold;
}

template<typename T>
const std::vector<int>& 
SwitchStatsConf<T>::error_list(const T& switch_key)
{
    return switch_stats_info[switch_key].errcode_list;
}

template<typename T>
bool 
SwitchStatsConf<T>::isExists(const T& switch_key, int error_code)
{
    if (is_init_)
    {
        CAutoRLock l(mutex_);
        if (is_init_)
        {
            std::vector<int> & error_list = switch_stats_info[switch_key].errcode_list;
            return std::binary_search(error_list.begin(), error_list.end(), error_code);
        }
    }

    return false;
}
*/

#endif
