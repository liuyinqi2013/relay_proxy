#include    "relay_api.h"
#include    "trpc_sys_exception.h"
#include    "trpc_socket_opt.h"
#include    "system_conf.h"

extern CSystemConf *g_sys_conf_ptr;

CRelayApi::CRelayApi(const char *idc, const char * ip, const int port, const int index, const int time_out)
    :
    CApiBase(idc, ip, port, index, time_out),
    _is_connecting(false),
    _is_head(false)
{
}

CRelayApi::~CRelayApi()
{
    fini(true);
}

void CRelayApi::_encode(const char * packet, const int packet_len, const void * other_info)
{
    assert(packet != NULL);
    
    memset(_process_buf, 0x00, sizeof(_process_buf));

    // 先写长度
    memcpy(_process_buf, &packet_len, sizeof(int));

    // 写buf
    memcpy(_process_buf + sizeof(int), packet, packet_len);

    _processed = 0;
    _process_total = packet_len + sizeof(int);
}

void CRelayApi::_decode(char * packet, int & packet_len)
{
    assert(packet != NULL);
    
    // 读长度
    memcpy(&packet_len, _process_buf, sizeof(int));

    memcpy(packet, _process_buf + sizeof(int), packet_len);
}

// 链接登录等处理
int CRelayApi::_init()
{
    trpc_debug_log("_init connect: %d, _fd: %d", _is_connecting, _fd);
    
    DEBUG_TRACE();
    int ret = -1;
    
    if (!_is_connecting 
            && _fd == INVALID_SOCKET)
    {      
        // 创建fd
        _fd = CSocketOpt::tcp_sock();

        // 设置为非阻塞
        CSocketOpt::set_nonblock(_fd, true);

        // 非阻塞链接
        CSocketOpt::nonblock_connect(_fd, _ip.c_str(), _port);

        CSocketOpt::set_keep_alive(_fd, 1, g_sys_conf_ptr->keepalive_idle());

        // 修改状态为初始化中
        _set_initing();

        ret = CONNECTING;

        _is_connecting = true;
    }
    else
    {
        // 为connect后的第二次触发，检查connect是否成功
        CSocketOpt::check_connect_status(_fd);

        // 连接完毕
        _is_connecting = false;

        // 连上就认为初始化结束，relay协议不需要登录
        ret = INIT_OK;
    }
    
    return ret;
}

// 关闭，注销等的处理
void CRelayApi::_fini(bool force_close)
{
    /*
    if (force_close)
    {
        _is_connecting = false;
        _is_head = false;
    
        CSocketOpt::close(_fd);
    }
    */

    _is_connecting = false;
    _is_head = false;

    CSocketOpt::close(_fd);

    _is_init = false;
}

int CRelayApi::_recv_head()
{
    DEBUG_TRACE();
    
    int ret = -1;
    
    // 可以读，先读4个字节长度
    int size = -1;
    size = CSocketOpt::read(_fd, _process_buf, sizeof(int));

    if (size < (int)sizeof(int))
    {
        // 应该不可能出现4个字节都收不完的情况，直接报错
        THROW(CSocketException, ERR_IPC_ERROR, "read size error: %d < %lu", size, sizeof(int));

    }
    else
    {
        // 正常读取，设置buf长度
        // 不做网络字节序转换，坚持原接口
        memcpy(&_process_total, _process_buf, sizeof(int));

        // 确定长度范围
        if ((_process_total <= 0)
                || (_process_total > (int)(sizeof(_process_buf) - sizeof(int))))
        {
            THROW(CServerException, ERR_PACKET_LENGTH, "req packet len error: %d", _process_total);
        }

        // 已处理的数据包含开始的4个字节头
        _processed = sizeof(int);

        // 总数据长度为数据包长度 + sizeof(int) 包头
        _process_total += sizeof(int);
        
        ret = RECV_HAVE_MORE;
    }

    trpc_debug_log("recv_head ok, total length: %d", _process_total);

    _is_head = true;
    
    return ret;
}

int CRelayApi::_recv()
{
    DEBUG_TRACE();
    
    int ret = -1;
    if (!_is_head)
    {
        // 如果没有收包头，先收包头
        ret = _recv_head();
    }

    // 调用基类收包程序
    ret = CSocketBase::_recv();
    trpc_debug_log("after recv length: %d, left len: %d, buf is: [%s]",
                    _process_total, _processed, _process_buf + sizeof(int));

    return ret;
}
