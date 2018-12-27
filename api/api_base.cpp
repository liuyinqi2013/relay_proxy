#include    "api_base.h"
#include    "trpc_socket_opt.cpp"
#include    "trpc_sys_exception.h"

CApiBase::CApiBase(const char *idc, const char * ip, const int port, const int index, const int time_out)
    :
    CSocketBase(),
    _status(EM_FREE),
    _index(index),
    _is_init(false),
    _is_used(false),
    _req_packet(NULL),
    _req_len(0),
    _other_info(NULL),
    _client_seq_no(0),
    _idc(idc),
    _ip(ip),
    _port(port),
    _time_out(time_out)
{
}

CApiBase::~CApiBase()
{
    //fini(true);
}

int CApiBase::process()
{
    int ret = -1;
   
    switch(_status)
    {
        case EM_READY:
        case EM_FREE:
            DEBUG_TRACE();

        case EM_INITING:
            DEBUG_TRACE();
            ret = init();
            if (ret == INIT_OK)
            {           
                trpc_debug_log("ip: %s, port: %d, req_packet: %s", ip(), port(), _req_packet);
                
                // ���� -- init�׶ο��ܻ���Ϊ�е�¼�������⣬����buf
                _encode(_req_packet, _req_len, _other_info);
                
                // ��ʼ���ɹ���api����send�׶Σ����ﲻ��Ҫ��break��ֱ�ӳ��Է���һ��
                _status = EM_SEND;
            }
            /*
            else
            {
                break;
            }
            */

            break;
        case EM_SEND:
            DEBUG_TRACE();
            ret = _send();
            if (ret == SEND_OK)
            {
                // �����������ȴ��Է��Ż�
                _status = EM_READ;

                // send ����Ժ���������Ϣ
                _processed = 0;
                _process_total = 0;

                memset(_process_buf, 0x00, sizeof(_process_buf));
            }
            break;
        case EM_READ:
            DEBUG_TRACE();
            ret = _recv();
            if (ret == RECV_OK)
            {
                // �޸�״̬Ϊ�������ȴ�����
                _status = EM_READ_OK;
            }
            break;
        default:
            THROW(CTrpcException, ERR_SYS_UNKNOWN, "status error: %d", _status);
    }

    return ret;
}

void CApiBase::encode(const char * packet, const int packet_len, const void * other_info)
{
    assert(_status == EM_FREE || _status == EM_OVER);
    assert(_req_packet == NULL);
    assert(_req_len == 0);
    assert(_other_info == NULL);

    // ����packet
    _req_packet = packet;
    _req_len = packet_len;
    _other_info = other_info;
    
    // ���ﲻ�����룬�ڳ�ʼ������Ժ���������

    _status = EM_READY;
}

void CApiBase::decode(char * packet, int& packet_len)
{
    assert(_status == EM_READ_OK);
    
    _decode(packet, packet_len);

    trpc_debug_log("ip: %s, port: %d, res_packet: %s", ip(), port(), packet);
    
    // �������
    _status = EM_OVER;
}

int CApiBase::init()
{   
    if (_is_init)
    {
        return INIT_OK;
    }

    int ret = _init();
    
    if (ret == INIT_OK)
    {
        _is_init = true;
    }

    return ret;
}

void CApiBase::fini(bool force_close)
{
    // finiһ���ɹ�����ʧ��
    _fini(force_close);

    _req_packet = NULL;
    _req_len = 0;
    _other_info = NULL;

    _client_seq_no = 0;

    _processed = 0;
    _process_total = 0;

    _status = EM_FREE;
    
    /*
    if (force_close)
    {
        _is_init = false;
    }
    */
}

const int CApiBase::index()
{
    return _index;
}

bool CApiBase::is_free()
{
    return _is_used == false;
}

void CApiBase::set_used()
{
    assert(!_is_used);

    if (!CSocketOpt::check_tcp_ok(_fd))
    {
        trpc_debug_log("socket fd was closed by peer!");
        fini(true);
    }

    _is_used = true;
}

void CApiBase::set_free(bool force_close)
{
    fini(force_close);
    
    // ����״̬��ǿ������Ϊ����
    _is_used = false;
}

