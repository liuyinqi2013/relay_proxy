#ifndef     __API_POOL_H__
#define     __API_POOL_H__

#include    <vector>
#include    <string>

#include    "api_base.h"
#include    "hasite_conf.h"
#include    "system_conf.h"
//#include    "key_service_stats.h"
#include    "thread_mutex.h"

// api pool
class CApiPool
{
// Õë¶ÔÃ¿¸öip
public:
    CApiPool(CHaSiteConf& hasite_conf, CSystemConf& sys_conf, const std::string& server_name, 
                    const int conn_num, const int max_conn_num, const int time_out);
    ~CApiPool();
    
    CApiBase * get(const char * user, const char * passwd);
    void release(CApiBase * api, bool force_close);

    void  set_switch_on();
    void  set_switch_off();

    /*
    void  set_service_stats_pool(ServiceStatsPool *pool)
    {
        _service_stats_pool = pool;
    }
    */

    const std::string& server_name() const
    {
        return _server_name;
    }

    bool need_switch() const
    {
        return _need_switch;
    }

protected:
    CApiBase *  _get(const char * user, const char * passwd);
    void        _release(CApiBase * api, bool force_close);

    CApiBase *  _add_api(const char * user, const char * passwd);

    CApiBase * _create_api(const std::string& idc,
                          const std::string& ip,
                          int                port,
                          int                protocol,
                          const char         *user,
                          const char         *passwd);
    
protected:
    enum API_SWITCH_STATE
    {
        EM_SWITCH_PRIOR,
        EM_SWITCH_ONGOING,
        EM_SWITCH_FINISHED,
    };
protected:
    std::string _server_name;
    std::string _ip_addr;
    int         _port;

    bool        _need_switch;

    int     _conn_num;
    int     _max_conn_num;
    CHaSiteConf&    _hasite_conf;
    CSystemConf&    _sys_conf;
    int     _time_out;

    CRwLock     _mutex;
    int          _pool_size;
    std::vector<CApiBase *>     *_api_pool;

    std::vector<CApiBase *>     _pool[2];
    int                         _switch_factor;
    int                         _switch_state;
};

#endif

