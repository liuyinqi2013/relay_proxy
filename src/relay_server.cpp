#include    <list>
#include    "relay_server.h"
#include    "trpc_sys_exception.h"
#include    "relay_log.h"
#include    "trpc_socket_opt.h"
#include    "trpc_url_param_map.h"
#include    "relay_mgr_command.h"

// 沿用以前的方式，采用相对路径
const char * CRelayServer::SYSTEM_CONF = "./conf/SysConfig.conf";
const char * CRelayServer::REQUEST_TYPE_CONF = "./conf/RequestAppServer.conf";
const char * CRelayServer::HASITE_CONF = "./conf/hasite.xml";
const char * CRelayServer::CORE_SERVER_CONF = "./conf/CoreServer.conf";
const char * CRelayServer::ACCESS_CONF = "./conf/AuthAddr.conf";
const char * CRelayServer::FRELIMIT_CONF = "./conf/FreqLimit.conf";
const char * CRelayServer::ERRCODE_ESCAPE_CONF = "./conf/ErrInfo.conf";
const char * CRelayServer::SERVICE_STATS_CONF = "./conf/service_switch_stats.conf";
const char * CRelayServer::SITE_STATS_CONF = "./conf/site_switch_stats.conf";
const char * CRelayServer::DETAIL_INFO_SERVICE_CONF = "./conf/DetailInfoRequestType.conf";
const char * CRelayServer::SERVICE_REQUEST_CONTROL_CONF = "./conf/service_request_control.conf";
const char * CRelayServer::SERVICE_AUTHORIZE_ACCESS_CONF = "./conf/service_authorize_addr.conf";

ErrorCodeEscapeConf g_errcode_escape_conf;
DetailInfoServiceConf g_detail_info_service_conf;
CSystemConf *g_sys_conf_ptr = NULL;

CRelayServer::CRelayServer()
    :
    _process_thread(NULL)
{
}

CRelayServer::~CRelayServer()
{
    fini();
}

void CRelayServer::init()
{
    // 读取配置
    _read_conf();   

    g_sys_conf_ptr = &_sys_conf;

    //_service_stats_pool.init(_interface_stats_conf);

    // 初始化api连接池
    //_core_server_conf.init_api_pool(_hasite_conf, _sys_conf, &_service_stats_pool);
    _core_server_conf.init_api_pool(_hasite_conf, _sys_conf);

    _relay_switch_mgr.init(&_core_server_conf, &_hasite_conf, &_service_stats_conf, &_site_stats_conf, &_sys_conf);

    #ifdef RULE_FROM_FILE
    trpc_debug_log("RULE_FROM_FILE");
    _service_request_control.init(CRelayServer::SERVICE_REQUEST_CONTROL_CONF);
    #endif

    _service_authorize_access.init(CRelayServer::SERVICE_AUTHORIZE_ACCESS_CONF);

    // 这里开始可以使用日志
    // 先初始化处理线程
    _init_process_thread();

    _init_relay_mgr_thread();
    
    // 初始化bind的socket
    _init_accept_socket();
}

void CRelayServer::fini()
{
    _fini_process_thread();
    
    _fini_accept_socket();    

    fini_log();
}

void CRelayServer::reload_conf()
{
    // 依次调用其他配置的配置加载函数
    
    // request_type配置
    _request_type_conf.clear();

    for (std::vector<CSystemConf::ST_RequestKeyConf>::const_iterator t_key = _sys_conf.request_key().begin();
            t_key !=  _sys_conf.request_key().end(); ++t_key)
    {
        _request_type_conf.read_conf(t_key->conf_file.c_str());
    }

    // access 配置
    _access_conf.clear();
    _access_conf.read_conf(ACCESS_CONF);

    // 频率限制
    _freqlimit_conf.clear();
    _freqlimit_conf.read_conf(FRELIMIT_CONF);

    // 错误码转义配置
    g_errcode_escape_conf.clean();
    g_errcode_escape_conf.read_conf(ERRCODE_ESCAPE_CONF);

    // 明细信息配置
    g_detail_info_service_conf.reload(DETAIL_INFO_SERVICE_CONF);

    _service_authorize_access.re_init(SERVICE_AUTHORIZE_ACCESS_CONF);

    _hasite_conf.reload(HASITE_CONF);
    _core_server_conf.re_init(CORE_SERVER_CONF, _hasite_conf, _sys_conf);

    _service_stats_conf.re_init(SERVICE_STATS_CONF);
    _site_stats_conf.re_init(SITE_STATS_CONF);

    _relay_switch_mgr.re_init(&_core_server_conf, &_hasite_conf, &_service_stats_conf, &_site_stats_conf, &_sys_conf);
}

void CRelayServer::process()
{
    // 先启动处理线程
    for (int i = 0; i < _process_thread_num; ++i)
    {
        CEventThread& thread = _process_thread[i]._thread;  

        thread.run();

        trpc_debug_log("process thread %d is running", i);
    }

    // 启动监听线程
    _accept_thread.run();

    _run_over();
}

void CRelayServer::_run_over()
{
    // 暂时等待accept线程结束
    // 后续的管理等处理在这里做
    if(pthread_join(_accept_thread.thread_id(), NULL) == 0) {
        trpc_error_log("accept_thread is quit");
    } else {
        trpc_error_log("accept_thread is error quit %s", strerror(errno));
    }
}

void CRelayServer::_read_conf()
{
    _sys_conf.init(SYSTEM_CONF);

    if (_sys_conf.log_with_lock())
    {
        trpc_set_log_mutex(&_log_mutex);
        trpc_set_acclog_mutex(&_acc_log_mutex);
        trpc_set_statslog_mutex(&_stats_log_mutex);
    }

    // 读取多个request_type文件的数据
    _request_type_conf.clear();

    for (std::vector<CSystemConf::ST_RequestKeyConf>::const_iterator t_key = _sys_conf.request_key().begin();
            t_key !=  _sys_conf.request_key().end(); ++t_key)
    {
        trpc_debug_log("file:%s", t_key->conf_file.c_str());
        _request_type_conf.read_conf(t_key->conf_file.c_str());
    }

    _core_server_conf.read_conf(CORE_SERVER_CONF);

    _hasite_conf.read_conf(HASITE_CONF);

    _access_conf.read_conf(ACCESS_CONF);

    _freqlimit_conf.read_conf(FRELIMIT_CONF);

    g_errcode_escape_conf.read_conf(ERRCODE_ESCAPE_CONF);

    _service_stats_conf.init(SERVICE_STATS_CONF);

    _site_stats_conf.init(SITE_STATS_CONF);

    g_detail_info_service_conf.read_conf(DETAIL_INFO_SERVICE_CONF);
}

void CRelayServer::_init_accept_socket()
{
    // 初始化绑定的socket
    _accept_fd = CSocketOpt::tcp_sock();

    CSocketOpt::set_reuser_addr(_accept_fd);
    
    CSocketOpt::net_bind(_accept_fd, _sys_conf.server_ip(), _sys_conf.server_port());

    CSocketOpt::nonblock_listen(_accept_fd, 128);//ximi add 128 at 20160822

    // 初始化
    _accept_thread.init();

    trpc_debug_log("accept_fd: %d", _accept_fd);
    
    // 加入到监听队列
    _accept_thread.add_event(_accept_fd, EV_READ | EV_PERSIST, CRelayServer::_client_accept_handler, this, 0);
}

void CRelayServer::_fini_accept_socket()
{
    CSocketOpt::close(_accept_fd);
}

void CRelayServer::_init_process_thread()
{
    _process_thread_num = _sys_conf.thread_num();

    trpc_debug_log("create process thread threads num: %d", _process_thread_num);
    
    _process_thread = new ST_ThreadData[_process_thread_num];

    // 初始化每个处理线程的管道
    for (int i = 0; i < _process_thread_num; ++i)
    {
        int pipe_fd[2];
        
        if (pipe(pipe_fd) < 0)
        {
            THROW(CTrpcException, ERR_SYS_UNKNOWN, "thread_%d's pipe create error: %s",
                        i, strerror(errno));
        }

        _process_thread[i]._write_fd = pipe_fd[1];
        _process_thread[i]._read_fd = pipe_fd[0];
        _process_thread[i]._thread_no = i;

        trpc_debug_log("add event process thread_%d, read_fd: %d", i, _process_thread[i]._read_fd);

        // 执行每个线程的初始化函数
        _process_thread[i]._thread.init();
                
        // 绑定到监听的管道上
        _process_thread[i]._thread.add_event(_process_thread[i]._read_fd, 
                                             EV_READ | EV_PERSIST, 
                                             CRelayServer::_process_thread_handle, 
                                             this, 
                                             0);
    }
}

void CRelayServer::_fini_process_thread()
{
    if (_process_thread == NULL)
    {
        // 线程还未初始化，不需要调用析构函数
        return;
    }
    
    for (int i = 0; i < _process_thread_num; ++i)
    {
        CSocketOpt::close(_process_thread[i]._write_fd);
        CSocketOpt::close(_process_thread[i]._read_fd);

        bool is_created = _process_thread[i]._thread.is_created();

        _process_thread[i]._thread.fini();

        // 强制杀死所有的子线程
        if (is_created)
        {
            if (pthread_cancel(_process_thread[i]._thread.thread_id()) != 0) 
            {
                trpc_error_log("thread_%d pthread_cancel error: %s", i, strerror(errno));
            
                continue;
            }
        
            if(pthread_join(_process_thread[i]._thread.thread_id(), NULL) == 0) 
            {
                trpc_debug_log("thread_%d pthread_join error: %s", i, strerror(errno));
            
                continue;
            }
        
            trpc_debug_log("thread_%d exit success", i);
        }
    }

    // 这里delete会挂掉，比较奇怪
    delete []_process_thread;
}

int CRelayServer::_client_accept_handler(int fd, int event, void * param, CEventBase * event_base)
{
    CRelayServer * me = (CRelayServer *)param;

    try
    {
        me->_process_accept(fd);
    }
    catch(CTrpcException& ce)
    {        
        // 出错 -- 不关闭监听的socket
        // CTrpcSocketOpt::close(fd);   
        // 如果是EAGAIN 不报错
        if (ce.error() != ERR_SOCKET_AGAIN)
        {
            trpc_error_log("error: %s", ce.error_info());
        }
    }
    catch(std::exception & ce)
    {
        trpc_error_log("std error: %s", ce.what());
    }
    catch(...)
    {
        trpc_error_log("unknown exception");
    }

    return 0;
}

void CRelayServer::_process_accept(int fd)
{
    assert(fd == _accept_fd);

    int client_fd = CSocketOpt::accept(fd);

    DEBUG_TRACE();
    
    if (client_fd < 0)
    {
        // 可重入错误，抛出异常等待下次监听
        THROW(CServerException, ERR_SOCKET_AGAIN, "accept error: %s", strerror(errno));
    }

    trpc_debug_log("accept ok : fd, %d", client_fd);

    // 检查ip限制 -- 暂时先预留
    
    // 指定fd查找
    CClientSocket& client_socket = _client_socket(client_fd);

    try
    {
        // 发送给对应的线程
        _choose_thread(client_fd, client_socket);
    }
    catch(...)
    {
        // accept这里出错暂时不返回错误信息
        CSocketOpt::close(client_fd);

        CAutoRLock  l(_client_data_mutex);
        client_socket.fini();
    
        throw;
    }
}

// 选择对应的线程
void CRelayServer::_choose_thread(int fd, CClientSocket& client_socket)
{
    DEBUG_TRACE();
    
    static int thread_index = 0;
    static unsigned int seq_no = 0;

    ST_ThreadData& thread = _process_thread[thread_index++];

    if (thread_index >= _process_thread_num - 1)
    {
        // 重置为0
        thread_index = 0;
    }

    // 设置socket的fd和thread_no -- thread_no 参考用
    client_socket.init(fd, thread._thread_no, ++seq_no, this);

    // 发送到管道
    ST_ThreadPipeMsg msg;
    
    msg._fd = fd;
    msg._seq_no = client_socket.seq_no();

    int count = write(thread._write_fd, (char *)&msg, sizeof(msg));
    if (count < 0)
    {
        THROW(CIpcException, ERR_IPC_ERROR, "write thread_%d's pipe: %d, error: %s",
                    thread._thread_no, thread._write_fd, strerror(errno));
    }
    else if (count != sizeof(msg))
    {
        // 写管道出错，直接抛异常
        THROW(CIpcException, ERR_IPC_ERROR, "write thread_%d's pipe: %d, size error: %d != %lu",
                    thread._thread_no, thread._write_fd, count, sizeof(msg));
    }
}

int CRelayServer::_process_thread_handle(int fd, int event, void * param, CEventBase * event_base)
{
    DEBUG_TRACE();
    
    CRelayServer * me = (CRelayServer *)param;

    try
    {
        me->_process_dispatch(fd);
    }
    catch(CTrpcException& ce)
    {         
        // 如果是EAGAIN 不报错
        if (ce.error() != ERR_SOCKET_AGAIN)
        {
            trpc_error_log("error: %s", ce.error_info());
        }
    }
    catch(std::exception & ce)
    {
        trpc_error_log("std error: %s", ce.what());
    }
    catch(...)
    {
        trpc_error_log("unknown exception");
    }

    return 0;
}

// 处理转发
void CRelayServer::_process_dispatch(int fd)
{
    DEBUG_TRACE();
    
    // 接收消息fd
    ST_ThreadPipeMsg msg;
    
    int count = read(fd, (char *)(&msg), sizeof(msg));
    if (count < 0)
    {
        THROW(CIpcException, ERR_IPC_ERROR, "read pipe: %d, error: %s", fd, strerror(errno));
    }
    else if (count != sizeof(msg))
    {
        // 写管道出错，直接抛异常
        THROW(CIpcException, ERR_IPC_ERROR, "read pipe: %d, size error: %d != %lu", fd, count, sizeof(msg));
    }

    // 获取内存信息
    CClientSocket& client_socket = _client_socket(msg._fd);

    // 从这里开始已经获取了socket，后续处理出现异常需要做socket的释放
    try
    {
        // 检查序列号是否相等
        if (msg._seq_no != client_socket.seq_no())
        {
            // 可能是fd被复用了
            THROW(CIpcException, ERR_IPC_ERROR, "packet seq_no not match -- pipe: %u != socket: %u",
                  msg._seq_no, client_socket.seq_no());
        }

        CEventThread& thread = _process_thread[_get_thread_no(client_socket)]._thread;  

        // 已经获取了
        // 检测ip受限权限
        _access_conf.check(client_socket.ip());

        thread.add_event(client_socket.fd(), EV_READ|EV_TIMEOUT, CRelayServer::_dispatch_handler,   
                         this, _sys_conf.cgi_write_time_out());
    }
    catch(CTrpcException& ce)
    {      
        trpc_error_log("error: %s", ce.error_info());

        if (client_socket.status() != CClientSocket::EM_SEND)
        {
            // 返回错误信息给client
            _write_error(client_socket, ce.error(), ce.what());
        }
        else
        {
            // 如果是发送过程出错，直接关闭socket即可
            CAutoRLock  l(_client_data_mutex);
            client_socket.fini();
        }

        throw;
    }
    catch(std::exception & ce)
    {
        trpc_error_log("std error: %s", ce.what());

        if (client_socket.status() != CClientSocket::EM_SEND)
        {
            // 返回错误信息给client
            _write_error(client_socket, ERR_SYS_UNKNOWN, ce.what());
        }
        else
        {
            // 如果是发送过程出错，直接关闭socket即可
            CAutoRLock  l(_client_data_mutex);
            client_socket.fini();
        }

        throw;
    }
    catch(...)
    {
        const char * unknown_exception = "unknown exception";
        trpc_error_log(unknown_exception);
        
        if (client_socket.status() != CClientSocket::EM_SEND)
        {
            // 返回错误信息给client
            _write_error(client_socket, ERR_SYS_UNKNOWN, unknown_exception);
        }
        else
        {
            // 如果是发送过程出错，直接关闭socket即可
            CAutoRLock  l(_client_data_mutex);
            client_socket.fini();
        }

        throw;
    }
}

// 处理client端的请求
void CRelayServer::_thread_dispatch(CClientSocket& client_socket, 
	                                    CEventThread& thread, 
	                                    int socket_status, 
	                                    CEventBase * & event_base)
{
    DEBUG_TRACE();
    
    // 代码中检查
    if (client_socket.status() != socket_status)
    {
        THROW(CTrpcException, ERR_SYS_UNKNOWN, "client_socket status: %d, now: %d",
                                    client_socket.status(), socket_status);
    }
    
    int ret = 0;

    ret = client_socket.process();

    trpc_debug_log("client fd: %d, process ret: %d, status: %d",
                        client_socket.fd(), ret, client_socket.status());
    
    DEBUG_TRACE();
   
    // 检查返回结果
    switch(ret)
    {
        case CClientSocket::RECV_HAVE_MORE:
        {
            // 需要继续收 -- 不需要做任何处理，已经在epoll中
            break;
        }
        case CClientSocket::RECV_OK:
        {
            trpc_debug_log("delete event fd: %d, event: %d", event_base->fd(), event_base->event());
            
            // 从epoll集合中删除，调用后台server接口
            thread.del_event(event_base);
            
            DEBUG_TRACE();
            
            // 解包
            client_socket.decode(_sys_conf.request_key(), _sys_conf.relay_svr_no());

            // 检测ip + request_type的限制
            _access_conf.check(client_socket.ip(), client_socket.request_type());

            if (!_service_authorize_access.authorized(client_socket.request_type(), client_socket.ip()))
            {
                THROW(CServerException, InvalidAccess, 
                      "ip:%s is not authorized to access request_type:%d", 
                      client_socket.ip(), client_socket.request_type());
            }

            service_access_control_check(client_socket);

            /*
            // 检查ip + request_type的频率的限制
            _freqlimit_conf.check(client_socket.ip(), client_socket.request_type());
            */

            CRequestTypeConf::ST_RequestConfData request_conf_item;
            _request_type_conf.get(client_socket.request_key(), request_conf_item);
            _relay_switch_mgr.incrRequest(request_conf_item.site_no, request_conf_item.service_name);

            // 处理转发业务请求
            _call_server(client_socket);
            
            break;
        }
        case CClientSocket::SEND_HAVE_MORE:
        {
            // 继续接收，不需要做处理
            break;
        }
        case CClientSocket::SEND_OK:
        {
            thread.del_event(event_base);

            if (!client_socket.is_system_error())
            {
                //write acclog
                CRequestTypeConf::ST_RequestConfData &request_conf = client_socket.request_conf();
                trpc_write_acclog("%s|%s|%s|%s|%d|%d|%d|%d|%d|%s|%-.128s \n",
                               _sys_conf.server_ip(), client_socket.ip(), request_conf.service_name, client_socket.server_ip(),
                               client_socket.total_process_microseconds(), 0, client_socket.call_middle_microseconds(), 0, 
                               0, client_socket.req_packet(), client_socket.acc_res_packet().c_str()); 

            }

            // 处理完毕 -- socket回收
            CAutoRLock  l(_client_data_mutex);
            client_socket.fini();

            break;
        }
        default:
        {
            // 现在recv_close直接抛异常出去了，不需要单独处理
            THROW(CServerException, ERR_SYS_UNKNOWN, "process ret errot: %d", ret);
        }
    }
}

int CRelayServer::_dispatch_handler(int fd, int event, void * param, CEventBase * event_base)
{
    DEBUG_TRACE();

    trpc_debug_log("process fd: %d, event: 0x%08x", fd, event);
    
    CRelayServer * me = (CRelayServer *)param;

    // 获取相关的client_socket
    CClientSocket& client_socket = me->_client_socket(fd); 
    
    CEventThread& thread = me->_process_thread[me->_get_thread_no(client_socket)]._thread;

    try
    {
        // 检查是不是超时
        if (check_mask(event, EV_READ))
        {
            // 处理接收请求
            me->_thread_dispatch(client_socket, thread, CClientSocket::EM_READ, event_base);
        } 
        else if(check_mask(event, EV_WRITE))
        {
            // 处理超时请求
            me->_thread_dispatch(client_socket, thread, CClientSocket::EM_SEND, event_base);
        }
        else if (check_mask(event, EV_TIMEOUT))
        {
            // 这里的话只检查读超时，读超时直接关闭即可
            THROW(CServerException, ERR_SOCKET_TIME_OUT, "ip: %s, port: %d, read time out, fd: %d",
                    client_socket.ip(), client_socket.port(), client_socket.fd());
        }
        else
        {
            // 未知信号, 系统异常，直接报错
            THROW(CTrpcException, ERR_SYS_UNKNOWN, "event error: %d", event);
        }
    }
    catch(CTrpcException& ce)
    {      
        trpc_error_log("error: %s", ce.error_info());

        //if (event_base != NULL)
        if (event_base != NULL && event_base->is_in_queue())
        {
            thread.del_event(event_base);
        }

        if (client_socket.status() != CClientSocket::EM_SEND)
        {
            // 返回错误信息给client
            me->_write_error(client_socket, ce.error(), ce.what());
        }
        else
        {
            // 如果是发送过程出错，直接关闭socket即可
            CAutoRLock  l(me->_client_data_mutex);
            client_socket.fini();
        }
    }
    catch(std::exception & ce)
    {
        trpc_error_log("std error: %s", ce.what());
        
        //if (event_base != NULL)
        if (event_base != NULL && event_base->is_in_queue())
        {
            thread.del_event(event_base);
        }
        
        if (client_socket.status() != CClientSocket::EM_SEND)
        {
            // 返回错误信息给client
            me->_write_error(client_socket, ERR_SYS_UNKNOWN, ce.what());
        }
        else
        {
            // 如果是发送过程出错，直接关闭socket即可
            CAutoRLock  l(me->_client_data_mutex);
            client_socket.fini();
        }
    }
    catch(...)
    {
        const char * unknown_exception = "unknown exception";
        trpc_error_log(unknown_exception);
        
        //if (event_base != NULL)
        if (event_base != NULL && event_base->is_in_queue())
        {
            thread.del_event(event_base);
        }
        
        if (client_socket.status() != CClientSocket::EM_SEND)
        {
            // 返回错误信息给client
            me->_write_error(client_socket, ERR_SYS_UNKNOWN, unknown_exception);
        }
        else
        {
            // 如果是发送过程出错，直接关闭socket即可
            CAutoRLock  l(me->_client_data_mutex);
            client_socket.fini();
        }
    }

    return 0;
}

int CRelayServer::_service_handler(int fd, int event, void * param, CEventBase * event_base)
{
    DEBUG_TRACE();
    
    CClientSocket * p = (CClientSocket *)param;
    CRelayServer * me = (CRelayServer *)p->relay();

    // 获取相关的client_socket
    CClientSocket& client_socket = *p; 
    CApiBase * api = client_socket.api();
    CEventThread& thread = me->_process_thread[me->_get_thread_no(client_socket)]._thread;

    try
    {
        // 检查是不是超时
        if (check_mask(event, EV_READ))
        {
            // 处理接收请求
            me->_service_dispatch(client_socket, api, CClientSocket::EM_READ, event_base);
        } 
        else if(check_mask(event, EV_WRITE))
        {
            // 处理超时请求
            me->_service_dispatch(client_socket, api, CClientSocket::EM_SEND, event_base);
        }
        else if (check_mask(event, EV_TIMEOUT))
        {
            // 这里的话只检查读超时，读超时直接关闭即可
            THROW(CServerException, ERR_SOCKET_TIME_OUT, "ip: %s, port: %d, read time out, fd: %d",
                    client_socket.ip(), client_socket.port(), client_socket.fd());
        }
        else
        {
            // 未知信号, 系统异常，直接报错
            THROW(CTrpcException, ERR_SYS_UNKNOWN, "event error: %d", event);
        }
    }
    catch(CTrpcException& ce)
    {        
        trpc_error_log("error: %s", ce.error_info());
        
        //if (event_base != NULL)
        if (event_base != NULL && event_base->is_in_queue())
        {
            thread.del_event(event_base);
        }
        
        // 释放并关闭api
        api->set_free(true);
        client_socket.api(NULL);
        //me->_core_server_conf.free_api(client_socket.request_conf().site_no, api, true);
        
        // 返回错误信息给client
        me->_write_error(client_socket, ce.error(), ce.what());
    }
    catch(std::exception & ce)
    {
        trpc_error_log("std error: %s", ce.what());
        
        //if (event_base != NULL)
        if (event_base != NULL && event_base->is_in_queue())
        {
            thread.del_event(event_base);
        }
        
        // 释放并关闭api
        api->set_free(true);
        client_socket.api(NULL);
        //me->_core_server_conf.free_api(client_socket.request_conf().site_no, api, true);
        
        me->_write_error(client_socket, ERR_SYS_UNKNOWN, ce.what());
    }
    catch(...)
    {
        const char * unknown_exception = "unknown exception";
        trpc_error_log(unknown_exception);
        
        //if (event_base != NULL)
        if (event_base != NULL && event_base->is_in_queue())
        {
            thread.del_event(event_base);
        }
        
        // 释放并关闭api
        api->set_free(true);
        client_socket.api(NULL);
        //me->_core_server_conf.free_api(client_socket.request_conf().site_no, api, true);
        
        me->_write_error(client_socket, ERR_SYS_UNKNOWN, unknown_exception);
    }

    return 0;
}

void CRelayServer::_call_server(CClientSocket& client_socket)
{
    DEBUG_TRACE();

    // 根据request_key查找对应的站点信息
    _request_type_conf.get(client_socket.request_key(), client_socket.request_conf());

    // 查找对应的server连接池
    
    CApiBase *  api = _core_server_conf.get_api(client_socket.request_conf().site_no);

    trpc_debug_log("dst IP:%s, PORT:%d, index=%d\n", api->ip(), api->port(), api->index());

    client_socket.server_ip(api->ip());
    client_socket.server_port(api->port());

    api->client_seq_no(client_socket.seq_no());

    // 编码
    api->encode(client_socket.req_packet(), client_socket.req_packet_len(), client_socket.request_conf().service_name);

    trpc_debug_log("client_socket fd: %d, thread_no: %d", client_socket.fd(), client_socket.thread_no());

    // 设置api指针
    client_socket.api(api);
    
    // 开始写
    // 先做请求处理
    try
    {
        CEventBase *tmp = NULL;
        _service_dispatch(client_socket, api, CClientSocket::EM_SEND, tmp);
    }
    catch(CTrpcException& ce)
    {
        //_core_server_conf.free_api(client_socket.request_conf().site_no, api, true);
        api->set_free(true);
        client_socket.api(NULL);
       
        throw;
    }
}

void CRelayServer::_service_dispatch(CClientSocket& client_socket, 
                                     CApiBase * api, 
                                     int socket_status, 
                                     CEventBase * & event_base)
{
    DEBUG_TRACE();
    
    CTrpcUrlParamMap param;
    int result = 0;

    int ret = -1;

    trpc_debug_log("client_socket fd: %d, thread_no: %d", client_socket.fd(), client_socket.thread_no());

    CEventThread& thread = _process_thread[_get_thread_no(client_socket)]._thread;

    ret = api->process();
    
    trpc_debug_log("process api ret: %d, api fd:%d, client_socket fd: %d, thread_no: %d", 
                   ret, api->fd(), client_socket.fd(), client_socket.thread_no());

    switch(ret)
    {
        case CSocketBase::CONNECTING:
       
            // 链接中，同时监听读和写 -- 超时时间采用站点的超时时间
            thread.add_event(api->fd(), 
                             EV_READ | EV_TIMEOUT | EV_WRITE, 
                             CRelayServer::_service_handler,   
                             &client_socket, 
                             api->time_out());
            break;

        case CSocketBase::INIT_OK:
            // 初始化成功，开始发包
            if (event_base == NULL)
            {
                thread.add_event(api->fd(), 
                                 EV_WRITE | EV_TIMEOUT | EV_WRITE, 
                                 CRelayServer::_service_handler,   
                                 &client_socket, 
                                 api->time_out());
            }
            else
            { 
                thread.update_event(event_base, 
                                    EV_WRITE | EV_TIMEOUT, 
                                    CRelayServer::_service_handler, 
                                    &client_socket);
            }
            break;

        case CSocketBase::SEND_HAVE_MORE:
            // 继续发送，不需要做处理
            break;

        case CSocketBase::SEND_OK:
            assert(event_base != NULL);
            // 将读的fd监听修改为读
            thread.update_event(event_base, 
                                EV_READ | EV_TIMEOUT, 
                                CRelayServer::_service_handler, 
                                &client_socket);

            break;
        case CSocketBase::RECV_HAVE_MORE:
            // 继续接收，不需要做处理
            break;
        case CSocketBase::RECV_OK:
            // 将fd从monitor中删掉
            thread.del_event(event_base);
            
            if (api->client_seq_no() != client_socket.seq_no())
            {
                trpc_debug_log("Not the same seq_no, api:%u, client_socket:%u", 
                                api->client_seq_no(), client_socket.seq_no())

                 api->set_free(false);
                 break;
            }
            
            // 接收完毕，解包
            api->decode(client_socket.res_packet(), client_socket.res_packet_len());

            // 还回api -- 不需要强制关闭
            api->set_free(false);
            client_socket.api(NULL);

            result = client_socket.get_call_result();
            if (result != 0) 
            {
                CRequestTypeConf::ST_RequestConfData request_conf_item;
                _request_type_conf.get(client_socket.request_key(), request_conf_item);
                if (_relay_switch_mgr.trySwitch(request_conf_item.site_no, request_conf_item.service_name, result) != -1)
                {
                    trpc_write_statslog("[%s][%d]|OUT|%s|%s|%d|%s|%s|%s \n",
                                        __FILE__, __LINE__, _sys_conf.server_ip(), client_socket.ip(), 
                                        request_conf_item.site_no, request_conf_item.service_name, 
                                        client_socket.server_ip(), client_socket.acc_res_packet().c_str());
                }
            }

            // 将返回包写回client 
            client_socket.encode();
            thread.add_event(client_socket.fd(), 
                             EV_WRITE|EV_TIMEOUT, 
                             CRelayServer::_dispatch_handler,   
                             this, 
                             _sys_conf.cgi_write_time_out());            
            break;

        default:
            THROW(CTrpcException, ERR_SYS_UNKNOWN, "unknow api ret: %d", ret);
    }
}

void 
CRelayServer::service_access_control_check(CClientSocket& client_socket)
{
    int request_type = client_socket.request_type();
    CRequestTypeConf::ST_RequestConfData request_conf_item;
    _request_type_conf.get(client_socket.request_key(), request_conf_item);
    
    // 检查ip + request_type的频率的限制
    _freqlimit_conf.check(client_socket.ip(), request_type);

    CTrpcUrlParamMap req_param_map(client_socket.req_packet());
    std::string master_key = _sys_conf.master_request_key(request_type);
trpc_debug_log("master_key=%s, request_type=%d", master_key.c_str(), request_type);
    if (master_key.length() > 0)
    {
        _service_request_control.check_rule(req_param_map[master_key], 0, 
                                            request_conf_item.service_name, client_socket.req_packet());
    }
}

void CRelayServer::_write_error(CClientSocket& client_socket, const int error, const char * res_info)
{
    client_socket.set_system_error();
    client_socket.set_recv_time_stamp();
    client_socket.set_process_time_stamp();

    bool found = true;
    CRequestTypeConf::ST_RequestConfData request_conf_item;
    try
    {
        _request_type_conf.get(client_socket.request_key(), request_conf_item);
    }
    catch(CConfigException & ex)
    {
        trpc_debug_log("can't find request_type, %d, %s", ex.error(), ex.what());
        found = false;
    }

    /*
    if (found && _relay_switch_mgr.trySwitch(request_conf_item.site_no, request_conf_item.service_name, error) == 0)
    {
        trpc_write_statslog("[%s][%d]|OUT|%s|%s|%d|%s|%s|%s \n",
                            __FILE__, __LINE__, _sys_conf.server_ip(), client_socket.ip(), request_conf_item.site_no,
                            request_conf_item.service_name, client_socket.server_ip(), client_socket.acc_res_packet().c_str());
    }
    */

    CEventThread& thread = _process_thread[_get_thread_no(client_socket)]._thread;

    char * p = client_socket.res_packet();

    if (found && request_conf_item.protocol == EM_PROTOCOL_BIN)
    {
        memcpy(p, &error, sizeof(int));
        const int len = strlen(res_info);
        memcpy(p + sizeof(int), res_info, len);

        client_socket.res_packet_len() = sizeof(int) + len;
    }
    else
    {
        std::string res_packet;
    
        CTrpcUrlParamMap res;
        res["result"] = to_string(error);
        res["res_info"] = res_info;

        res.packUrl(res_packet);

        strncpy(p, res_packet.c_str(), CApiBase::MAX_PACKET_LEN - 1);
        *(p + CApiBase::MAX_PACKET_LEN - 1) = '\0';

        client_socket.res_packet_len() = (int)res_packet.length() > CApiBase::MAX_PACKET_LEN 
                                          ? CApiBase::MAX_PACKET_LEN - 1 
                                          : (int)res_packet.length();
    }

    if (found && _relay_switch_mgr.trySwitch(request_conf_item.site_no, request_conf_item.service_name, error) != -1)
    {
        trpc_write_statslog("[%s][%d]|OUT|%s|%s|%d|%s|%s|%s \n",
                            __FILE__, __LINE__, _sys_conf.server_ip(), client_socket.ip(), request_conf_item.site_no,
                            request_conf_item.service_name, client_socket.server_ip(), client_socket.acc_res_packet().c_str());
    }

    CRequestTypeConf::ST_RequestConfData& request_conf = client_socket.request_conf();
    trpc_write_acclog("%s|%s|%s|%s|%d|%d|%d|%d|%d|%s|%-.128s \n",
                      _sys_conf.server_ip(), client_socket.ip(), request_conf.service_name, client_socket.server_ip(),
                       client_socket.total_process_microseconds(), 0, client_socket.call_middle_microseconds(), 0, 
                       0, client_socket.req_packet(), client_socket.acc_res_packet().c_str()); 
    
    // 将返回包写回client 
    client_socket.encode();
    
    //_thread_dispatch(client_socket, thread, CClientSocket::EM_SEND, NULL);
    thread.add_event(client_socket.fd(), 
                     EV_WRITE|EV_TIMEOUT, 
                     CRelayServer::_dispatch_handler,   
                     this, 
                     _sys_conf.cgi_write_time_out());            
}

void *
CRelayServer::relay_mgr_worker(void *arg)
{
    trpc_debug_log("CRelayServer::relay_mgr_worker");
    CRelayServer *self = static_cast<CRelayServer *> (arg);

    self->relay_mgr_worker_i();

    return NULL;
}

void 
CRelayServer::relay_mgr_worker_i()
{
    trpc_debug_log("CRelayServer::relay_mgr_worker_i()");

    std::list<struct TimedCommand> command_list;
    create_relay_mgr_command(command_list, this);

    #ifndef RULE_FROM_FILE
        for(std::list<struct TimedCommand>::iterator iter = command_list.begin(); 
            iter != command_list.end(); 
            ++iter)
        {
            struct TimedCommand & time_cmd = *iter;
            time_cmd.command_->process();
        }
    #endif

    for(;;)
    {
        #ifdef RULE_FROM_FILE
            _service_request_control.re_init(CRelayServer::SERVICE_REQUEST_CONTROL_CONF);
            sleep(60);
        #else
            struct timeval tv;
            tv.tv_sec = latest_expire_command_time(command_list);
            tv.tv_usec = 0;

            if (select(0, NULL, NULL, NULL, &tv) == 0)
            {
                for(std::list<struct TimedCommand>::iterator iter = command_list.begin(); 
                    iter != command_list.end(); 
                    ++iter)
                {
                    time_t now = time(0);    
                    struct TimedCommand & time_cmd = *iter;
                    if (now - time_cmd.time_stamp_ >= time_cmd.time_out_)
                    {
                        trpc_debug_log("command time out, cmd_type=%d", time_cmd.cmd_type_);
                        time_cmd.time_stamp_ = now;
                        time_cmd.command_->process();
                    }
                    else
                    {
                        trpc_debug_log("command NOT time out, cmd_type=%d, now - time_cmd.time_stamp_=%d, time_out=%d", 
                                       time_cmd.cmd_type_, (int)(now - time_cmd.time_stamp_), time_cmd.time_out_);
                    }
                }
            }
        #endif
    }
}

void
CRelayServer::_init_relay_mgr_thread()
{
    pthread_t thr_id;
    if(pthread_create(&thr_id, NULL, CRelayServer::relay_mgr_worker, (void *)this) != 0) 
    {
        THROW(CThreadException, ERR_SYS_UNKNOWN, "pthread_create error: %s", strerror(errno));
    }
}
