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

    // ��д����
    memcpy(_process_buf, &packet_len, sizeof(int));

    // дbuf
    memcpy(_process_buf + sizeof(int), packet, packet_len);

    _processed = 0;
    _process_total = packet_len + sizeof(int);
}

void CRelayApi::_decode(char * packet, int & packet_len)
{
    assert(packet != NULL);
    
    // ������
    memcpy(&packet_len, _process_buf, sizeof(int));

    memcpy(packet, _process_buf + sizeof(int), packet_len);
}

// ���ӵ�¼�ȴ���
int CRelayApi::_init()
{
    trpc_debug_log("_init connect: %d, _fd: %d", _is_connecting, _fd);
    
    DEBUG_TRACE();
    int ret = -1;
    
    if (!_is_connecting 
            && _fd == INVALID_SOCKET)
    {      
        // ����fd
        _fd = CSocketOpt::tcp_sock();

        // ����Ϊ������
        CSocketOpt::set_nonblock(_fd, true);

        // ����������
        CSocketOpt::nonblock_connect(_fd, _ip.c_str(), _port);

        CSocketOpt::set_keep_alive(_fd, 1, g_sys_conf_ptr->keepalive_idle());

        // �޸�״̬Ϊ��ʼ����
        _set_initing();

        ret = CONNECTING;

        _is_connecting = true;
    }
    else
    {
        // Ϊconnect��ĵڶ��δ��������connect�Ƿ�ɹ�
        CSocketOpt::check_connect_status(_fd);

        // �������
        _is_connecting = false;

        // ���Ͼ���Ϊ��ʼ��������relayЭ�鲻��Ҫ��¼
        ret = INIT_OK;
    }
    
    return ret;
}

// �رգ�ע���ȵĴ���
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
    
    // ���Զ����ȶ�4���ֽڳ���
    int size = -1;
    size = CSocketOpt::read(_fd, _process_buf, sizeof(int));

    if (size < (int)sizeof(int))
    {
        // Ӧ�ò����ܳ���4���ֽڶ��ղ���������ֱ�ӱ���
        THROW(CSocketException, ERR_IPC_ERROR, "read size error: %d < %lu", size, sizeof(int));

    }
    else
    {
        // ������ȡ������buf����
        // ���������ֽ���ת�������ԭ�ӿ�
        memcpy(&_process_total, _process_buf, sizeof(int));

        // ȷ�����ȷ�Χ
        if ((_process_total <= 0)
                || (_process_total > (int)(sizeof(_process_buf) - sizeof(int))))
        {
            THROW(CServerException, ERR_PACKET_LENGTH, "req packet len error: %d", _process_total);
        }

        // �Ѵ�������ݰ�����ʼ��4���ֽ�ͷ
        _processed = sizeof(int);

        // �����ݳ���Ϊ���ݰ����� + sizeof(int) ��ͷ
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
        // ���û���հ�ͷ�����հ�ͷ
        ret = _recv_head();
    }

    // ���û����հ�����
    ret = CSocketBase::_recv();
    trpc_debug_log("after recv length: %d, left len: %d, buf is: [%s]",
                    _process_total, _processed, _process_buf + sizeof(int));

    return ret;
}
