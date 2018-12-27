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

// relay �����ȳ���
class CRelayServer
{
public:
    CRelayServer();
    ~CRelayServer();

    void    init();
    void    fini();

    void    process();

    // ���������ļ�
    void    reload_conf();

protected:
    void    _read_conf();

    void    _init_accept_socket();
    void    _fini_accept_socket();

    void    _init_process_thread();
    void    _fini_process_thread();

    CClientSocket& _client_socket(int fd)
    {
        // ��ʱֻ�Ӷ���
        CAutoRLock  l(_client_data_mutex);
        
        return _client_data[fd];
    }

    void    _choose_thread(int fd, CClientSocket& client_socket);

    const int _get_thread_no(CClientSocket& client_socket)
    {
        int thread_no = client_socket.thread_no();
        
        if (thread_no < 0)
        {
            // ��У��һ����ʱ
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
    // �������
    void    _process_accept(int fd);

    // ����middle����
    void    _call_server(CClientSocket& client_socket);

    // ����ת�� process_thread_handle ������
    void    _process_dispatch(int fd);

    // ����client����
    void    _thread_dispatch(CClientSocket& client_socket, CEventThread& thread, int socket_status, CEventBase * & event_base);

    // _service_handle��ʵ�ʴ�����
    void    _service_dispatch(CClientSocket& client_socket, CApiBase * api, int socket_status, CEventBase * & event_base);

    // �߳��������ס������������
    void    _run_over();

    // ���ش�����Ϣ��client
    void    _write_error(CClientSocket& client_socket, const int error, const char * res_info);
    
protected:
    // ����client_accept����
    static int _client_accept_handler(int fd, int event, void * param, CEventBase * event_base);   

    // ����pipe����
    static int _process_thread_handle(int fd, int event, void * param, CEventBase * event_base);

    // ����client��Ϣ
    static int _dispatch_handler(int fd, int event, void * param, CEventBase * event_base);

    // ����service����
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

        // thread ��Ҫ����ӡ��־����������¼ʹ��
        int             _thread_no;
        
        // ͨѶ�ܵ�
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

    // ������Ϣ
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

    // ������Ϣ
    int     _accept_fd;

    // �����߳�
    CEventThread    _accept_thread;

    // �����߳�
    int             _process_thread_num;
    ST_ThreadData * _process_thread;
    
    // ��������map��, ����ǰһ������fdΪkey
    std::map<int, CClientSocket>    _client_data;
    
    CRwLock         _client_data_mutex;

    CLogMutex       _acc_log_mutex;
    CLogMutex       _log_mutex;
    CLogMutex       _stats_log_mutex;

};

#endif

