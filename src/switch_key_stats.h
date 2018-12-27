
#ifndef __SWITCH_KEY_STATIS_H__
#define __SWITCH_KEY_STATIS_H__

#include <string>
#include <map>

#include <sstream>

#include "log.h"
//#include    "thread_mutex.h"
#include    "switch_stats_conf.h"

template<typename T>
class SwitchKeyStats
{
public:
    SwitchKeyStats(const T& key_name, const std::string& key_id, int time_interval, 
                   int request_threshold, int succ_ratio, std::vector<int> error_list);
    SwitchKeyStats();
    ~SwitchKeyStats();

    //void increment_requests(const std::string& idc);
    //void increment_errors(const std::string& idc);

    void reset(const std::string& idc);

    void increment_requests();
    void increment_errors();

    void reset();

    bool need_switch();

    bool is_exist(int error);

    std::string key_id() const
    {
        return key_id_;
    }

    bool is_same_id(const std::string& key_id) const
    {
        return key_id == key_id_;
    }

private:
    //CRwLock     mutex_; 
    time_t      begin_timestamp_;
    int         requests_;
    int         errors_;

    bool        to_be_switched_;

    int         time_interval_;
    int         minimal_request_threshold_;
    int         minimal_succ_ratio_;
    std::vector<int>  errcode_list_;

    //std::string service_name_;
    T           key_name_;

    std::string key_id_;
};

template<typename T>
class SwitchStatsPool
{
public:
    SwitchStatsPool();
    ~SwitchStatsPool();

    bool is_switch_key(const T& switch_key);

    void increment_requests(const T& switch_key);
    void increment_errors(const T& switch_key);
    void reset();
    void reset(const T& switch_key);

    //void increment_requests(const char *idc, const char *service_name);
    //void increment_errors(const char *idc, const char *service_name);
    //void reset(const char *idc, const char *service_name);

    bool need_switch(const T& switch_key);
    bool is_exist(const T& switch_key, int error);

    void init(SwitchStatsConf<T> & conf);

    void clear();

private:
    std::map<T, SwitchKeyStats<T> > switch_key_stats_map_;
};

typedef SwitchStatsPool<std::string>  ServiceStatsPool;
typedef SwitchStatsPool<int>  SiteStatsPool;


//SwitchKeyStats::SwitchKeyStats(const std::string& service_name, int time_interval, int request_threshold, int succ_ratio) :
template<typename T>
SwitchKeyStats<T>::SwitchKeyStats(const T& key_name, const std::string& key_id, 
                                  int time_interval, int request_threshold, 
                                  int succ_ratio, std::vector<int> error_list) :
begin_timestamp_(time(0)),
requests_(0),
errors_(0),
to_be_switched_(false),
time_interval_(time_interval),
minimal_request_threshold_(request_threshold),
minimal_succ_ratio_(succ_ratio),
errcode_list_(error_list),
key_name_(key_name),
key_id_(key_id)
{
}

//SwitchKeyStats::SwitchKeyStats()
template<typename T>
SwitchKeyStats<T>::SwitchKeyStats()
{
}

template<typename T>
SwitchKeyStats<T>::~SwitchKeyStats()
//SwitchKeyStats::~SwitchKeyStats()
{
}

template<typename T>
void
SwitchKeyStats<T>::increment_requests()
{
    time_t now = time(0);

    //CAutoWLock l(mutex_);

    if (now - begin_timestamp_ < time_interval_)
    {
        if (!to_be_switched_)
        {
            ++requests_;
        }
    }
    else
    {
        //trpc_debug_log("key-service:%s, increment_request, over interval", service_name_.c_str());
        if (!to_be_switched_)
        {
            trpc_debug_log("now:%ld - begin_timestamp:%ld = %ld > time_interval:%d, over the time interval, reset",
                            now, begin_timestamp_, now - begin_timestamp_, time_interval_);
            if (requests_ > 0)
            {
                float ratio = ((float)((requests_ - errors_)*100))/requests_;
                if (ratio < 0) ratio = 0;
                
                std::string key_str = type_cast<T, std::string>(key_name_);
                trpc_write_statslog("%s|begin_timestamp:%ld|requests:%d|errors:%d|succ_ratio:%.2f\%|\n",
                                     key_str.c_str(), begin_timestamp_, requests_, errors_, ratio);
            }

            requests_ = 1;
            errors_ = 0;
            begin_timestamp_ = now;
        }
        else
        {
            //trpc_debug_log("NEED_TO_BE_SWITCHED");
        }
    }
}

template<typename T>
void
SwitchKeyStats<T>::increment_errors()
{
    time_t now = time(0);

    //CAutoWLock l(mutex_);
    if (now - begin_timestamp_ < time_interval_)
    {
        if (!to_be_switched_)
        {
            if (requests_ == 0)
            {
                requests_ = 1;
                errors_ = 1;
            }
            else
            {
                ++errors_;
            }

            //trpc_debug_log("ADDING ERRORS:%d\n", errors_);
            float ratio = ((float)((requests_ - errors_)*100))/requests_;
            if (ratio < 0) ratio = 0;

            if (requests_ >= minimal_request_threshold_
                && ratio <= minimal_succ_ratio_)
            {
                trpc_debug_log("TOO MANY ERRORS, SWITCHING, requests:%d, request_threshold:%d, suc_rate:%.2f, minimal_ratio:%d",
                                requests_, minimal_request_threshold_, ratio, minimal_succ_ratio_);

                std::ostringstream oss;
                oss << key_name_;
                trpc_write_statslog("%s|SET_SWITCH|begin_timestamp:%d|requests:%d|errors:%d|succ_ratio:%.2f\%|\n", 
                                     oss.str().c_str(), begin_timestamp_, requests_, errors_, ratio);
                to_be_switched_ = true;
            }
        }
    }
    else
    {
        //trpc_debug_log("key-service:%s, increment_error, over interval", service_name_.c_str());
        if (!to_be_switched_)
        {
            trpc_debug_log("over time interval, reset");
            if (requests_ > 0)
            {
                float ratio = ((float)((requests_ - errors_)*100))/requests_;
                if (ratio < 0) ratio = 0;
                std::ostringstream oss;
                oss << key_name_;
                trpc_write_statslog("%s|begin_timestamp:%d|requests:%d|errors:%d|succ_ratio:%.2f\%|\n", 
                                    oss.str().c_str(), begin_timestamp_, requests_, errors_,  ratio);
            }
            requests_ = 1;
            errors_ = 1;
            begin_timestamp_ = now;
        }
        else
        {
            //trpc_debug_log("SET SWITCHED\n");
        }
    }
}

template<typename T>
void
SwitchKeyStats<T>::reset()
{
    //CAutoWLock l(mutex_);
    
    requests_ = 0;
    errors_ = 0;
    begin_timestamp_ = time(0);
    to_be_switched_ = false;

    trpc_write_statslog("reset key_id[%s], requests=%d, error=%d, begin_timestamp=%ld, switched=%d \n",
                        key_id_.c_str(), requests_, errors_, begin_timestamp_, to_be_switched_);
}

template<typename T>
bool
SwitchKeyStats<T>::need_switch()
{
    //CAutoRLock l(mutex_);

    return to_be_switched_;
}

template<typename T>
bool
SwitchKeyStats<T>::is_exist(int error)
{
    //CAutoRLock l(mutex_);
    return std::binary_search(errcode_list_.begin(), errcode_list_.end(), error);
}

template<typename T>
SwitchStatsPool<T>::SwitchStatsPool()
{
}

template<typename T>
SwitchStatsPool<T>::~SwitchStatsPool()
{
}

template<typename T>
void
SwitchStatsPool<T>::init(SwitchStatsConf<T> & conf)
{
    //CAutoWLock l(conf.mutex_);
    std::map<T, struct stats_conf>& switch_stats_conf = conf.switch_stats_info;
    for(typename std::map<T, struct stats_conf>::const_iterator iter = switch_stats_conf.begin(); 
        iter != switch_stats_conf.end();
        ++iter)
    {
        const T& key = (*iter).first;
        const struct stats_conf &conf = (*iter).second;

        SwitchKeyStats<T> stat(key, conf.key_id, conf.time_interval, conf.minimal_request_threshold, 
                               conf.minimal_success_ratio, conf.errcode_list);
        switch_key_stats_map_[key] = stat; 

        std::string key_str = type_cast<T, std::string>(key);
        trpc_debug_log("key:%s, time_interval:%d, minimal_request_threshold:%d, minimal_success_ratio:%d",
                        key_str.c_str(), conf.time_interval, conf.minimal_request_threshold, conf.minimal_success_ratio);
    }
}

template<typename T>
bool
SwitchStatsPool<T>::is_switch_key(const T& switch_key)
{
    return switch_key_stats_map_.find(switch_key) != switch_key_stats_map_.end();
}

template<typename T>
void 
SwitchStatsPool<T>::increment_requests(const T& switch_key)
{
    typename std::map<T, SwitchKeyStats<T> >::iterator iter = switch_key_stats_map_.find(switch_key);
    if (iter != switch_key_stats_map_.end())
    {
        (*iter).second.increment_requests();
    }
    else
    {
        std::string key_str = type_cast<T, std::string>(switch_key);
        trpc_debug_log("not found %s for increment_requests",  key_str.c_str());
    }
}

template<typename T>
void 
SwitchStatsPool<T>::increment_errors(const T& switch_key)
{
    typename std::map<T, SwitchKeyStats<T> >::iterator iter = switch_key_stats_map_.find(switch_key);
    if (iter != switch_key_stats_map_.end())
    {
        (*iter).second.increment_errors();
    }
    else
    {
        std::string key_str = type_cast<T, std::string>(switch_key);
        trpc_debug_log("not found %s for increment_errors",  key_str.c_str());
    }
}

template<typename T>
void 
SwitchStatsPool<T>::reset(const T& switch_key)
{
    std::string key_str = type_cast<T, std::string>(switch_key);
    trpc_write_statslog("reseting key service:%s\n", key_str.c_str());

    if (switch_key_stats_map_.count(switch_key) > 0)
    {
        SwitchKeyStats<T>& stat = switch_key_stats_map_[switch_key];
        std::string key_id = stat.key_id();

        for(typename std::map<T, SwitchKeyStats<T> >::iterator iter = switch_key_stats_map_.begin();
            iter != switch_key_stats_map_.end();
            ++iter)
        {
            SwitchKeyStats<T>& stat = (*iter).second;
            if (stat.is_same_id(key_id))
            {
                (*iter).second.reset();
            }
        }
    }

    return;
}

template<typename T>
void 
SwitchStatsPool<T>::reset()
{
    trpc_write_statslog("reseting for all switch_key");
    for(typename std::map<T, SwitchKeyStats<T> >::iterator iter = switch_key_stats_map_.begin();
        iter != switch_key_stats_map_.end();
        ++iter)
    {
        (*iter).second.reset();
    }
 
    return;
}

template<typename T>
void
SwitchStatsPool<T>::clear()
{
    switch_key_stats_map_.clear();
}

template<typename T>
bool 
SwitchStatsPool<T>::need_switch(const T& switch_key)
{
    typename std::map<T, SwitchKeyStats<T> >::iterator iter = switch_key_stats_map_.find(switch_key);
    if (iter != switch_key_stats_map_.end())
    {
        return (*iter).second.need_switch();
    }

    return false;
}

template<typename T>
bool
SwitchStatsPool<T>::is_exist(const T& switch_key, int error)
{
    typename std::map<T, SwitchKeyStats<T> >::iterator iter = switch_key_stats_map_.find(switch_key);
    if (iter != switch_key_stats_map_.end())
    {
        return (*iter).second.is_exist(error);
    }

    return false;
}

#endif
