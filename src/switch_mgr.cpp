
#include "hasite_conf.h"
#include "core_server_conf.h"
#include "log.h"
#include "switch_mgr.h"

RelaySwitchMgr::RelaySwitchMgr() :
service_stats_conf_(NULL),
site_stats_conf_(NULL),
core_server_conf_(NULL),
hasite_conf_(NULL),
sys_conf_(NULL),
is_service_init_(false),
is_site_init_(false)
{
}

RelaySwitchMgr::~RelaySwitchMgr()
{
    {
        CAutoWLock l(service_mutex_);

        is_service_init_ = false;
    }

    {
        CAutoWLock l(site_mutex_);

        is_site_init_ = false;
    }

    core_server_conf_ = NULL;
    hasite_conf_ = NULL ;
    service_stats_conf_ = NULL;
    site_stats_conf_= NULL;
    sys_conf_ = NULL;
}

void
RelaySwitchMgr::init(CCoreServerConf * coreserver_conf,
                     CHaSiteConf * hasite_conf,
                     ServiceStatsConf   *service_stats_conf,
                     SiteStatsConf   *site_stats_conf,
                     CSystemConf * sys_conf)
{
    assert(coreserver_conf != NULL && hasite_conf != NULL 
           && service_stats_conf != NULL && site_stats_conf != NULL && sys_conf != NULL);
    core_server_conf_ = coreserver_conf;
    hasite_conf_ = hasite_conf;
    service_stats_conf_ = service_stats_conf;
    site_stats_conf_ = site_stats_conf;
    sys_conf_ = sys_conf;

    {
        CAutoWLock l(service_mutex_);
        init_service_stats_pool(coreserver_conf, hasite_conf, service_stats_conf, sys_conf);
        is_service_init_ = true;
    }

    {
        CAutoWLock l(site_mutex_);
        init_site_stats_pool(coreserver_conf, hasite_conf, site_stats_conf, sys_conf);
        is_site_init_ = true;
    }
}

void
RelaySwitchMgr::init_service_stats_pool(CCoreServerConf * coreserver_conf,
                                        CHaSiteConf * hasite_conf,
                                        ServiceStatsConf   *service_stats_conf,
                                        CSystemConf * sys_conf)
{
    if (is_service_init_)
    {
        return;
    }

    service_stats_pool_.init(*service_stats_conf_);
}

void
RelaySwitchMgr::init_site_stats_pool(CCoreServerConf * coreserver_conf,
                                     CHaSiteConf * hasite_conf,
                                     SiteStatsConf   *site_stats_conf,
                                     CSystemConf * sys_conf)
{
    if (is_site_init_)
    {
        return;
    }

    site_stats_pool_.init(*site_stats_conf_);
}
void
RelaySwitchMgr::re_init(CCoreServerConf * coreserver_conf,
                        CHaSiteConf * hasite_conf,
                        ServiceStatsConf   *service_stats_conf,
                        SiteStatsConf   *site_stats_conf,
                        CSystemConf * sys_conf)
{
    {
        CAutoWLock l(service_mutex_);

        is_service_init_ = false;

        service_stats_pool_.clear();

        init_service_stats_pool(coreserver_conf, hasite_conf, service_stats_conf, sys_conf);

        is_service_init_ = true;
    }

    {
        CAutoWLock l(site_mutex_);

        is_site_init_ = false;

        site_stats_pool_.clear();

        init_site_stats_pool(coreserver_conf, hasite_conf, site_stats_conf, sys_conf);

        is_site_init_ = true;
    }
}

void 
RelaySwitchMgr::incrRequest(int site_no, const std::string& service_name)
{
    if(is_service_init_ && service_stats_pool_.is_switch_key(service_name.c_str()))
    {
        CAutoWLock l(service_mutex_);
        service_stats_pool_.increment_requests(service_name.c_str());
    }

    if (is_site_init_ && site_stats_pool_.is_switch_key(site_no))
    {
        CAutoWLock l(site_mutex_);
        site_stats_pool_.increment_requests(site_no);
    }
}

int
RelaySwitchMgr::trySwitch(int site_no, const std::string& service_name, int error)
{
    int ret = try_service_switch(site_no, service_name, error);
    if (ret != 1)
    {
        int ret1 = try_site_switch(site_no, service_name, error);
        if (ret1 == -1 && ret == -1)
        {
            return -1;
        } 
        else if (ret1 == 1)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return 1;
    }
}

//return 1 while needing switch, 
//0, while the service is key service, but not reach switch condition, 
//-1, other conditons
int
RelaySwitchMgr::try_service_switch(int site_no, const std::string& service_name, int error)
{
    bool switched = false;

    if (is_service_init_
        && service_stats_pool_.is_switch_key(service_name.c_str()) 
        && !service_stats_pool_.is_exist(service_name, error)) 
    {
        trpc_debug_log("RelaySwitchMgr::try_service_switch, site_no:%d, service_name:%s, error:%d\n",
                       site_no, service_name.c_str(), error);

        CAutoWLock l(service_mutex_);
        service_stats_pool_.increment_errors(service_name.c_str());
        if (service_stats_pool_.need_switch(service_name.c_str()))
        {
            switched = true;

            trpc_write_statslog("key service:%s reach switch condition\n", service_name.c_str());

            std::string server_name = core_server_conf_->server_name(site_no);
            std::string current_idc = hasite_conf_->get_current_idc(server_name.c_str());
            std::string next_idc = hasite_conf_->get_next_idc(server_name.c_str());

            if (next_idc == "")
            {
                trpc_write_statslog("site_no:%d current IDC:%s is the last IDC, do not switch\n",
                                     site_no, current_idc.c_str());
                service_stats_pool_.reset(service_name.c_str());
                //return 0;
                goto SWITCHED;
            }

            trpc_write_statslog("site_no:%d switch from IDC:%s to IDC:%s\n", 
                                site_no, current_idc.c_str(), next_idc.c_str());

            if (sys_conf_->is_switch_on())
            {
                if (core_server_conf_->_api_pool.count(site_no) > 0)
                {
                    trpc_write_statslog("site_no:%d set to be switched!\n", site_no);
                    core_server_conf_->_api_pool[site_no]->set_switch_on();
                }
            }

            service_stats_pool_.reset(service_name.c_str());
        }
    }
    else
    {
        return -1;
    }

SWITCHED:

    if (switched)
    {
        CAutoWLock l(site_mutex_);
        site_stats_pool_.reset(site_no);
        return 1;
    }
    
    return 0;
}

int
RelaySwitchMgr::try_site_switch(int site_no, const std::string& service_name, int error)
{
    bool switched = false;

    if (is_site_init_
        && site_stats_pool_.is_switch_key(site_no)
        && site_stats_pool_.is_exist(site_no, error)) 
    {
        CAutoWLock l(site_mutex_);
        site_stats_pool_.increment_errors(site_no);
        if (site_stats_pool_.need_switch(site_no))
        {
            switched = true;

            std::string server_name = core_server_conf_->server_name(site_no);
            std::string current_idc = hasite_conf_->get_current_idc(server_name.c_str());
            std::string next_idc = hasite_conf_->get_next_idc(server_name.c_str());

            if (next_idc == "")
            {
                trpc_write_statslog("site_no:%d current IDC:%s is the last IDC, do not switch\n",
                                     site_no, current_idc.c_str());
                site_stats_pool_.reset(site_no);
                //return 0;
                goto SWITCHED;
            }

            trpc_write_statslog("site_no:%d switch from IDC:%s to IDC:%s\n", 
                                site_no, current_idc.c_str(), next_idc.c_str());

            if (sys_conf_->is_switch_on())
            {
                if (core_server_conf_->_api_pool.count(site_no) > 0)
                {
                    trpc_write_statslog("site_no:%d set to be switched!\n", site_no);
                    core_server_conf_->_api_pool[site_no]->set_switch_on();
                }
            }

            site_stats_pool_.reset(site_no);
        }
    }
    else
    {
        return -1;
    }

SWITCHED:

    if (switched)
    {
        CAutoWLock l(service_mutex_);
        service_stats_pool_.reset(service_name);
        return 1;
    }

    return 0;
}
