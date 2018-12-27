
#include <stdio.h>
#include <stdlib.h>

#include "trpc_url_param_map.h"
#include "trpc_sys_exception.h"
#include "trpc_comm_func.h"
#include "service_request_control.h"
#include "service_fixed_control_rule.h"
#include "log.h"


ServiceRequestControl::ServiceRequestControl()
:is_init_(false)
{
}

ServiceRequestControl::~ServiceRequestControl()
{
    CAutoWLock l(mutex_);

    is_init_ = false;
    clean();
}

void
ServiceRequestControl::clean()
{
    for (std::map<std::string, ServiceControlRule *>::iterator i = service_control_rule_pool_.begin();
         i != service_control_rule_pool_.end();
         ++i)
    {
        ServiceControlRule * rule = (*i).second;
        delete rule;
    }

    service_control_rule_pool_.clear();
}

void
ServiceRequestControl::init(const char *service_control_rule_file)
{
    CAutoWLock l(mutex_);

    init_fix_i(service_control_rule_file);

    is_init_ = true;
}

void
ServiceRequestControl::init_fix_i(const char *service_control_rule_file)
{
    FILE *fp = fopen(service_control_rule_file, "r");

    if (fp == NULL)
    {
        THROW(CConfigException, ERR_CONF, "invalid config file:%s", service_control_rule_file);
    }

    char buffer[8096];
    std::vector<std::string> str_vec;
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        strstrip(buffer);
        if (strlen(buffer) <= 1) continue;
        if (buffer[0] == '#')    continue;

        create_fix_rule(buffer);
    }

    //is_init_ = true;
}

void
ServiceRequestControl::init(const std::vector<std::string>& rules)
{
    CAutoWLock l(mutex_);

    init_fix_i (rules);

    is_init_ = true;
}

void
ServiceRequestControl::init_fix_i(const std::vector<std::string>& rules)
{
    for(std::vector<std::string>::const_iterator i = rules.begin(); i != rules.end(); ++i)
    {
        create_fix_rule((*i));
    }

    //is_init_ = true;
}

void 
ServiceRequestControl::create_fix_rule(const std::string& one_rule)
{
    std::vector<std::string> str_vec;

    split(one_rule.c_str(), ":", 8, str_vec);
    size_t tokens = str_vec.size();
    if (tokens >= 5)
    {
        ServiceControlRule *scrule = new FixFreqControlRule(one_rule);
        
        std::map<std::string, ServiceControlRule *>::iterator iter = service_control_rule_pool_.find(str_vec[0]);
        if (iter != service_control_rule_pool_.end())
        {
            ServiceControlRule * rule = (*iter).second;
            delete rule;
        }

        trpc_debug_log("create rule:%s", str_vec[0].c_str());
        service_control_rule_pool_[str_vec[0]] = scrule;
    } 
    else
    {
        //THROW(CConfigException, ERR_CONF, "invalid service control rule[%s]", one_rule.c_str());
        trpc_error_log("invalid service control rule[%s]", one_rule.c_str());
    }
}

void
ServiceRequestControl::re_init(const char *service_control_rule_file)
{
    is_init_ = false;

    CAutoWLock l(mutex_);

    clean();

    init_fix_i(service_control_rule_file);

    is_init_ = true;
}

void
ServiceRequestControl::re_init(const std::vector<std::string>& rules)
{
    trpc_debug_log("re_init for request control rules");

    is_init_ = false;

    CAutoWLock l(mutex_);

    clean();

    init_fix_i(rules);

    is_init_ = true;
}


void 
ServiceRequestControl::check_rule(const std::string& master_key, int error, const char *service_name, const char *request_text)
{
    if (is_init_)
    {
        CAutoRLock l(mutex_);

        if (is_init_)
        {
            ServiceControlRule *rule = NULL;

            std::map<std::string, ServiceControlRule *>::iterator iter = service_control_rule_pool_.find(master_key);

            if (iter != service_control_rule_pool_.end())
            {
                trpc_debug_log("found, master_key===%s", master_key.c_str());
                rule = (*iter).second;

                rule->check_rule(error, service_name, request_text);
            }
        }
    }
}
