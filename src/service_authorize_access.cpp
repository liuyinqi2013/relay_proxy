
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>

#include "log.h"
#include "trpc_sys_exception.h"
#include "trpc_comm_func.h"
#include  "service_authorize_access.h"

ServiceAuthorizeAccess::ServiceAuthorizeAccess()
:is_init_(false)
{
}

ServiceAuthorizeAccess::~ServiceAuthorizeAccess()
{
}

void
ServiceAuthorizeAccess::init(const char *conf_file)
{
    CAutoWLock l(mutex_);
    
    init_i(conf_file);

    is_init_ = true;
}

void
ServiceAuthorizeAccess::init_i(const char *conf_file)
{
    if (is_init_)
    {
        return;
    }

    FILE *fp = fopen(conf_file, "r");

    if (fp == NULL)
    {
        THROW(CConfigException, ERR_CONF, "invalid config file:%s", conf_file);
    }

    char buffer[2048];
    std::vector<std::string> str_vec;
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        strstrip(buffer);
        if (strlen(buffer) <= 1) continue;
        if (buffer[0] == '#')    continue;

        split(buffer, ":", 4, str_vec);

        assert(str_vec.size() > 0);

        int request_type = atoi(str_vec[0].c_str());

        std::vector<std::string> ip_list;
        if (str_vec.size() > 1)
        {
            std::vector<std::string> ip_list;
            split(str_vec[1].c_str(), ",", 100, ip_list);
            std::sort(ip_list.begin(), ip_list.end());
            service_authorized_set_[request_type] = ip_list;
        }
        else
        {
            service_authorized_set_[request_type] = ip_list;
        }
    }
}

void
ServiceAuthorizeAccess::re_init(const char *conf_file)
{
    is_init_ = false;

    CAutoWLock l(mutex_);

    service_authorized_set_.clear();

    init_i(conf_file);

    is_init_ = true;
}

bool
ServiceAuthorizeAccess::authorized(int request_type, const char *access_ip)
{
    if (!is_init_)   
    {
        return true;
    }

    CAutoRLock l(mutex_);

    if (service_authorized_set_.count(request_type) == 0)
    {
        return true;
    }
    else
    {
        std::vector<std::string> & ip_list = service_authorized_set_[request_type];
        return std::binary_search(ip_list.begin(), ip_list.end(), access_ip);
    }
}
