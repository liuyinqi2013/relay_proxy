#ifndef     __DETAILED_INFO_SERVICE_H__
#define     __DETAILED_INFO_SERVICE_H__

#include    <set>
#include    "thread_mutex.h"

class DetailInfoServiceConf
{
public:
    DetailInfoServiceConf();
 
    void read_conf(const char *conf_file);

    void reload(const char * conf_flie);

    bool is_detail_info_service(int request_type);

private:
    CRwLock     _mutex;
    
    std::set<int>  detail_info_service_set_;
};

extern DetailInfoServiceConf g_detail_info_service_conf;
#endif

