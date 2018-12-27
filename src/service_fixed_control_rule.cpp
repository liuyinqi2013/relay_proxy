#include   <algorithm>

#include "trpc_comm_func.h"
#include   "log.h"
#include   "trpc_sys_exception.h"
#include   "trpc_url_param_map.h"

#include   "service_fixed_control_rule.h"

ServiceControlRule::ServiceControlRule()
{
}

ServiceControlRule::~ServiceControlRule()
{
}


FixFreqControlRule::FixFreqControlRule(const std::string& control_rule)
:ServiceControlRule()
{
    init(control_rule);
}
//   0          1         2          3                4                           5
//master_key:master_key_name:time,count:k1,...,kn:default_time,default_count:v1,...,vn,time,count|v1,...,vn,time,count
void
FixFreqControlRule::init(const std::string& control_rule)
{
    trpc_debug_log("control_rule=%s\n", control_rule.c_str());
    std::vector<std::string> rule_tokens;
    split(control_rule.c_str(), ":", 1000, rule_tokens);
    assert(rule_tokens.size() >= 5);

    master_key_ = rule_tokens[0];
    master_key_name_ = rule_tokens[1];

    std::vector<std::string> param_vec;
    split(rule_tokens[2].c_str(), ",", 8, param_vec);
    assert(param_vec.size() == 2);
    master_count_policy_.init(atoi(param_vec[0].c_str()), atoi(param_vec[1].c_str()));

    split(rule_tokens[3].c_str(), ",", 8, key_list_);
    size_t key_size = key_list_.size();

    std::vector<std::string> rule_vec;
    trpc_debug_log("rule_tokens[4]=%s", rule_tokens[4].c_str());
    split(rule_tokens[4].c_str(), ",", 4, rule_vec);
    assert(rule_vec.size() == 2);
    default_time_period_ = atoi(rule_vec[0].c_str());
    default_max_count_ = atoi(rule_vec[1].c_str());

    rule_vec.clear();
    split(rule_tokens[5].c_str(), "|", 1024, rule_vec);
    trpc_debug_log("rule_vec.size==%d", (int)rule_vec.size());

    for(size_t i = 0; i < rule_vec.size(); ++i)
    {
        std::vector<std::string> rule_item_vec;
        trpc_debug_log("rule_vec[%lu]===%s", i, rule_vec[i].c_str());
        split(rule_vec[i].c_str(), ",", key_size + 2, rule_item_vec);
        trpc_debug_log("rule_item_vec.size()===%d", (int)rule_item_vec.size());

        if (rule_item_vec.size() >= key_size)
        {
            std::vector<std::string> value_list;
            for (size_t i = 0; i < key_size; ++i)
            {
                value_list.push_back(rule_item_vec[i]);
            }
            int time_period = 0;
            int max_count = 0;
            if (rule_item_vec.size() >= key_size + 2)
            {
                time_period = atoi(rule_item_vec[key_size].c_str());
                max_count = atoi(rule_item_vec[key_size+1].c_str());
            }
            else
            {
                time_period = default_time_period_;
                max_count = default_max_count_;
            }

            struct Rule rule_node(value_list, time_period, max_count);

            rules_.push_back(rule_node);
        }
        else
        {
            trpc_error_log("invalid configure [%s] for %s in service_request_control.conf", 
                           rule_vec[i].c_str(), master_key_.c_str());
        }
    }

    /*
    for(size_t i = 0; i < rules_.size(); ++i)
    {
        rules_[i].print();
    }
    */
}

void 
FixFreqControlRule::check_rule(int error, const char * service_name, const char *request_text)
{
    trpc_debug_log("check bank:%s access control", master_key_name_.c_str());
    {
        //CAutoLock l(mutex_);
        if (master_count_policy_.increase_count() != 0)
        {
            THROW(CServerException, 
                  ExceedFreqLimit, 
                  "master_key:%s called too many times[%d] in %ds, over frequence limit:%d",
                  master_key_name_.c_str(), master_count_policy_.get_current_count(), 
                  (int)(time(0) - master_count_policy_.get_begin_timestamp()), 
                  master_count_policy_.get_max_count());
        }
    }

    trpc_debug_log("check %s access control", key_list_[0].c_str());

    CTrpcUrlParamMap param_map(request_text);
    std::vector<std::string> value_list;
    for(std::vector<std::string>::iterator iter = key_list_.begin(); iter != key_list_.end(); ++iter)
    {
        std::map<std::string, std::string>::iterator i = param_map.find(*iter);
        if (i != param_map.end())
        {
            value_list.push_back((*i).second); 
        }
        else 
        {
            if ((*iter) == "service_name")
            {
                 value_list.push_back(service_name);
            }
            else
            {
                break;
            }
        }
    }

    if (value_list.size() < key_list_.size())
    {
        trpc_debug_log("can't find some parameter from request_text");
        return;
    }

    Rule * rule = lookup(value_list);
    if (rule != NULL)
    {
        //CAutoLock l(rule->mutex_);

        if (rule->service_count_policy_.increase_count() != 0)
        {
            THROW(CServerException,
                  ExceedFreqLimit,
                  "bank:%s,%s:%s called too many times[%d] in %ds, over frequence limit:%d",
                   master_key_name_.c_str(), key_list_[0].c_str(), rule->value_list_[0].c_str(), 
                   rule->service_count_policy_.get_current_count(), 
                   (int)(time(0) - rule->service_count_policy_.get_begin_timestamp()), 
                   rule->service_count_policy_.get_max_count());
        }
    }
}

FixFreqControlRule::Rule * 
FixFreqControlRule::lookup(const std::vector<std::string>& value_list)
{
    struct Rule * rule = NULL;

    for(std::vector<struct Rule>::iterator i = rules_.begin(); i != rules_.end(); ++i)
    {
        if(value_list == (*i).value_list_)
        {
            rule = &(*i);
            break;
        }        
    }

    return rule;
}
