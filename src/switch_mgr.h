
#ifndef __RELAY_SWITCH_MANAGEMENT_H__
#define __RELAY_SWITCH_MANAGEMENT_H__

#include <string>

#include    "switch_stats_conf.h"
#include    "switch_key_stats.h"
#include    "system_conf.h"
#include    "thread_mutex.h"

class CCoreServerConf;
class CHaSiteConf;

class RelaySwitchMgr
{
public:
    RelaySwitchMgr();

    ~RelaySwitchMgr();

    //called when reloading config on-line
    void re_init(CCoreServerConf * coreserver_conf,
                   CHaSiteConf * hasite_conf,
                   ServiceStatsConf *service_stats_conf,
                   SiteStatsConf   *site_stats_conf,
                   CSystemConf * sys_conf);

    void init(CCoreServerConf * coreserver_conf,
                   CHaSiteConf * hasite_conf,
                   ServiceStatsConf *service_stats_conf,
                   SiteStatsConf   *site_stats_conf,
                   CSystemConf * sys_conf);


    void incrRequest(int site_no, const std::string& service_name);

    int  trySwitch(int site_no, const std::string& service_name, int error);

private:
    void init_service_stats_pool(CCoreServerConf * coreserver_conf,
                                 CHaSiteConf * hasite_conf,
                                 ServiceStatsConf *service_stats_conf,
                                 CSystemConf * sys_conf);

    void init_site_stats_pool(CCoreServerConf * coreserver_conf,
                                 CHaSiteConf * hasite_conf,
                                 SiteStatsConf   *site_stats_conf,
                                 CSystemConf * sys_conf);


    int  try_service_switch(int site_no, const std::string& service_name, int error);
    int  try_site_switch(int site_no, const std::string& service_name, int error);
private:
    ServiceStatsConf *   service_stats_conf_;
    SiteStatsConf    *   site_stats_conf_;
    CCoreServerConf *    core_server_conf_;
    CHaSiteConf     *    hasite_conf_;
    CSystemConf     *    sys_conf_;

    ServiceStatsPool   service_stats_pool_;
    bool               is_service_init_;
    CRwLock            service_mutex_;

    SiteStatsPool      site_stats_pool_;
    bool               is_site_init_;
    CRwLock           site_mutex_;
};

#endif
