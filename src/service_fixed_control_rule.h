
#ifndef __SERVICE_FIXED_CONTROL_RULE_H__
#define __SERVICE_FIXED_CONTROL_RULE_H__

#include    <vector>
#include    <map>
#include    <string>
#include    <sstream>
#include    <iomanip>

#include   "thread_mutex.h"
#include   "log.h"
#include   "time_count_policy.h"

class ServiceControlRule
{
public:
    ServiceControlRule();
    virtual ~ServiceControlRule() = 0;

    virtual void check_rule(int error, const char *service_name, const char *request_text) = 0;
};


class FixFreqControlRule : public ServiceControlRule
{
public:
    FixFreqControlRule(const std::string& control_rule);

    virtual void check_rule(int error, const char *service_name, const char *request_text);

protected:
    void init(const std::string& control_rule);

    //Rule * lookup(const std::vector<string>& value_list); 

private:
    std::string                   master_key_;
    std::string                   master_key_name_;

    //CThreadMutex                 mutex_;
    TimeCountPolicy              master_count_policy_;

    std::vector<std::string>     key_list_;

    struct Rule
    {
        //CThreadMutex                mutex_;
        std::vector<std::string>    value_list_;
        TimeCountPolicy             service_count_policy_;

        Rule(const std::vector<std::string>& value_list, int time_interval, int max_count):
        value_list_(value_list),
        service_count_policy_(time_interval, max_count)
        {
        }
    };

    std::vector<struct Rule>      rules_;
    int                           default_time_period_;
    int                           default_max_count_;

private:
    Rule * lookup(const std::vector<string>& value_list); 
};

#endif
