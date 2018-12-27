#ifndef     __RELAY_SERVER_H__
#define     __RELAY_SERVER_H__

#include    <string>
#include    <map>
#include    <vector>

#include    "event_thread.h"
#include    "client_socket.h"

#include    "system_conf.h"
#include    "request_type_conf.h"
#include    "hasite_conf.h"
#include    "core_server_conf.h"
#include    "access_control.h"
#include    "frequency_control.h"
#include    "errcode_escape_conf.h"
#include    "switch_stats_conf.h"
#include    "switch_key_stats.h"
#include    "detail_info_service_conf.h"
#include    "switch_mgr.h"
#include    "service_request_control.h"
#include    "service_authorize_access.h"
#include    "log_mutex.h"
#include    "relay_mgr_command.h"

// relay 主调度程序
class CRelayServer
{
public:
    CRelayServer();
    ~CRelayServer();

    void    init();
    void    fini();

    void    process();

    // 加载配置文件
    void    reload_conf();

protected:
    void    _read_conf();

    void    _init_accept_socket();
    void    _fini_accept_socket();

    void    _init_process_thread();
    void    _fini_process_thread();

    CClientSocket& _client_socket(int fd)
    {
        // 暂时只加读锁
        CAutoRLock  l(_client_data_mutex);
        
        return _client_data[fd];
    }

    void    _choose_thread(int fd, CClientSocket& client_socket);

    const int _get_thread_no(CClientSocket& client_socket)
    {
        int thread_no = client_socket.thread_no();
        
        if (thread_no < 0)
        {
            // 多校验一次暂时
            THROW(CTrpcException, ERR_SYS_UNKNOWN, "fd: %d, thread_no error: %d",
                        client_socket.fd(), thread_no);
        }

        return thread_no;
    }

protected:
    void  _init_relay_mgr_thread();
    static void *relay_mgr_worker(void *arg);

    void relay_mgr_worker_i();

protected:
    // 处理接收
    void    _process_accept(int fd);

    // 调用middle服务
    void    _call_server(CClientSocket& client_socket);

    // 处理转发 process_thread_handle 处理函数
    void    _process_dispatch(int fd);

    // 处理client请求
    void    _thread_dispatch(CClientSocket& client_socket, CEventThread& thread, int socket_status, CEventBase * & event_base);

    // _service_handle的实际处理函数
    void    _service_dispatch(CClientSocket& client_socket, CApiBase * api, int socket_status, CEventBase * & event_base);

    // 线程启动完后住函数做的事情
    void    _run_over();

    // 返回错误信息给client
    void    _write_error(CClientSocket& client_socket, const int error, const char * res_info);
    
protected:
    // 处理client_accept请求
    static int _client_accept_handler(int fd, int event, void * param, CEventBase * event_base);   

    // 处理pipe请求
    static int _process_thread_handle(int fd, int event, void * param, CEventBase * event_base);

    // 处理client信息
    static int _dispatch_handler(int fd, int event, void * param, CEventBase * event_base);

    // 处理service请求
    static int _service_handler(int fd, int event, void * param, CEventBase * event_base);

protected:
    void service_access_control_check(CClientSocket& client_socket);
    
protected:
    static const char * SYSTEM_CONF;
    static const char * REQUEST_TYPE_CONF;
    static const char * HASITE_CONF;
    static const char * CORE_SERVER_CONF;
    static const char * ACCESS_CONF;
    static const char * FRELIMIT_CONF;
    static const char * ERRCODE_ESCAPE_CONF;
    static const char * SERVICE_STATS_CONF;
    static const char * SITE_STATS_CONF;
    static const char * DETAIL_INFO_SERVICE_CONF;
    static const char * SERVICE_REQUEST_CONTROL_CONF;
    static const char * SERVICE_AUTHORIZE_ACCESS_CONF;

protected:
    class ST_ThreadData
    {
    public:
        CEventThread    _thread;

        // thread 需要，打印日志或者其他记录使用
        int             _thread_no;
        
        // 通讯管道
        int             _write_fd;
        int             _read_fd;
    };

    struct ST_ThreadPipeMsg
    {
        int             _fd;
        unsigned int    _seq_no;
    };
    
protected:

    friend class  ServiceFixFrequenceControlCommand;

    // 配置信息
    CSystemConf         _sys_conf;
    CRequestTypeConf    _request_type_conf;
    CHaSiteConf         _hasite_conf;
    CCoreServerConf     _core_server_conf;
    CAccessControl      _access_conf;
    CFrequencyControl   _freqlimit_conf;
    ErrorCodeEscapeConf _errcode_escape_conf;
    ServiceStatsConf    _service_stats_conf;
    SiteStatsConf       _site_stats_conf;
    //ServiceStatsPool   _service_stats_pool;
    //
    RelaySwitchMgr     _relay_switch_mgr;

    ServiceRequestControl  _service_request_control;
    ServiceAuthorizeAccess _service_authorize_access;

    // 其他信息
    int     _accept_fd;

    // 监听线程
    CEventThread    _accept_thread;

    // 处理线程
    int             _process_thread_num;
    ST_ThreadData * _process_thread;
    
    // 请求存放在map中, 和以前一样，以fd为key
    std::map<int, CClientSocket>    _client_data;
    
    CRwLock         _client_data_mutex;

    CLogMutex       _acc_log_mutex;
    CLogMutex       _log_mutex;
    CLogMutex       _stats_log_mutex;

};

#endif

