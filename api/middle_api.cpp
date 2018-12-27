#include    "middle_api.h"
#include    "log.h"
#include    "trpc_package.h"
#include    "trpc_sys_exception.h"
#include    "trpc_socket_opt.h"
#include    "system_conf.h"

extern CSystemConf *g_sys_conf_ptr;

CMiddleApi::CMiddleApi(const char *idc, const char * ip, const int port, const std::string& user, const std::string& passwd,
                    const int index, const int time_out)
    :
    CApiBase(idc, ip, port, index, time_out),
    _login_status(EM_NO_CONNECT),
    _is_head(false),
    _seq_no(0),
    _user(user),
    _passwd(passwd)
{
}

CMiddleApi::~CMiddleApi()
{
    fini(true);
}

void CMiddleApi::_encode(const char * packet, const int packet_len, const void * other_info)
{
    const char * service = (const char *)other_info;
    
    // �������ƴ��
    CTrpcPackage inPack;
    inPack.SetVersion(CTrpcPackage::VERSION);
    inPack.SetSeqNo(_seq_no++);
    inPack.SetMsgType(CTrpcPackage::TYPE_CALL_REQUEST);
    inPack.SetServiceName(service);
    inPack.SetResult(0);
    inPack.SetDataLen(packet_len);
    inPack.SetData(packet);

    if (inPack.Encode(_process_buf, _process_total, sizeof(_process_buf)) != 0)
    {
        THROW(CServerException, ERR_PACKET_ERROR, "encode packet error: %s", inPack.get_error_text());
    }

    _processed = 0;
}

void CMiddleApi::_decode(char * packet, int& packet_len)
{
    assert(packet != NULL);
    
    CTrpcPackage outPack;

    // ����
    if (outPack.Decode(_process_buf, _process_total) != 0)
    {
        // �������
        THROW(CServerException, ERR_PACKET_ERROR, "Decode: %s", outPack.get_error_text());
    }

    // ��鷵�ذ����
    if (outPack.GetMsgType() != CTrpcPackage::TYPE_CALL_RESPOND) 
    {
        THROW(CServerException, ERR_PACKET_ERROR, "Respond MsgType(%d) != TYPE_LOGIN_RESPOND",
                outPack.GetMsgType());
    }

    if (outPack.GetResult() != 0) 
    {
        // ִ��ʧ��
        THROW(CServerException, ERR_SERVER_ERROR, "Login error: %s",
                outPack.GetData());
    }

    packet_len = outPack.GetDataLen();
    memcpy(packet, outPack.GetData(), packet_len);
}

// ���ӵ�¼�ȴ���
int CMiddleApi::_init()
{
    int ret = -1;

    // �ȼ���Ƿ��Ѿ���ʼ����
    switch(_login_status)
    {
        case EM_NO_CONNECT:
        {      
            assert(_fd == INVALID_SOCKET);
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

            _login_status = EM_CONNECTING;

            break;
        }
        case EM_CONNECTING:
        {
            // Ϊconnect��ĵڶ��δ��������connect�Ƿ�ɹ�
            CSocketOpt::check_connect_status(_fd);

            // �����¼��
            _encode_login_packet();
            
            // ���͵�¼��
            _login_status = EM_SENDING;
            
            ret = SEND_HAVE_MORE;
            
            // ���ӳɹ��Ժ󣬼�������������break
        }
        case EM_SENDING:
        {
            // ���ӳɹ������͵�½��
            ret = _send();

            if (ret != SEND_OK)
            {
                break;
            }

            // �Ѿ�������ϣ����buf
            _processed = 0;
            _process_total = 0;

            _login_status = EM_RECVING;

            // ����������������
            break;
        }
        case EM_RECVING:
        {
            ret = _recv();

            if (ret != RECV_OK)
            {
                break;
            }

            // ���
            _decode_login_packet();

            trpc_debug_log("login ok: %s: %d, user: %s, pass: %s", ip(), port(), _user.c_str(), _passwd.c_str());

            // ������Ļ������ǵ�¼������

            ret = INIT_OK;
            
            break;
        }
        default:
        {
            THROW(CServerException, ERR_SYS_UNKNOWN, "unknown login status: %d", _login_status);
        }
    }
    
    return ret;
}

// �رգ�ע���ȵĴ���
void CMiddleApi::_fini(bool force_close)
{
    if (force_close)
    {
        _login_status = EM_NO_CONNECT;
        _is_head = false;
        _seq_no = 0;
    
        CSocketOpt::close(_fd);

        _is_init = false;
    }
}

// ��¼�������
void CMiddleApi::_encode_login_packet()
{
    // ��װ��¼��
    Trpc_Login_Body_T stLogin;
    strncpy(stLogin.sUserName, _user.c_str(), sizeof(stLogin.sUserName));
    strncpy(stLogin.sPasswd, _passwd.c_str(), sizeof(stLogin.sPasswd));
    stLogin.sUserName[sizeof(stLogin.sUserName) - 1] = '\0';
    stLogin.sPasswd[sizeof(stLogin.sPasswd) - 1] = '\0';

    CTrpcPackage inPack;
    inPack.SetVersion(CTrpcPackage::VERSION);
    inPack.SetSeqNo(_seq_no++);
    inPack.SetMsgType(CTrpcPackage::TYPE_LOGIN_REQUEST);
    inPack.SetServiceName("");
    inPack.SetResult(0);
    inPack.SetDataLen(sizeof(stLogin));
    inPack.SetData((const char *) &stLogin);

    if (inPack.Encode(_process_buf, _process_total, sizeof(_process_buf)) != 0)
    {
        THROW(CServerException, ERR_PACKET_ERROR, "encode packet error: %s", inPack.get_error_text());
    }
    
    _processed = 0;
}

void CMiddleApi::_decode_login_packet()
{
    CTrpcPackage outPack;

    // ����
    if (outPack.Decode(_process_buf, _process_total) != 0)
    {
        // �������
        THROW(CServerException, ERR_PACKET_ERROR, "Decode: %s", outPack.get_error_text());
    }

    // ��鷵�ذ����
    if (outPack.GetMsgType() != CTrpcPackage::TYPE_LOGIN_RESPOND) 
    {
        THROW(CServerException, ERR_PACKET_ERROR, "Respond MsgType(%d) != TYPE_LOGIN_RESPOND",
                outPack.GetMsgType());
    }

    if (outPack.GetResult() != 0) 
    {
        // ִ��ʧ��
        THROW(CServerException, ERR_SERVER_ERROR, "Login error: %s",
                outPack.GetData());
    }

    // ��¼�ɹ�
    _login_status = EM_LOGIN_OK;
}

int CMiddleApi::_recv_head()
{
    DEBUG_TRACE();
    
    int ret = -1;
    
    // ���Զ����ȶ�5���ֽڳ���
    int size = -1;
    size = CSocketOpt::read(_fd, _process_buf, 5);

    if (size < 5)
    {
        // Ӧ�ò����ܳ���5���ֽڶ��ղ���������ֱ�ӱ���
        THROW(CSocketException, ERR_IPC_ERROR, "read size error: %d < 5", size);
    }
    else
    {
        // ����ʽ
        if (_process_buf[0] != CTrpcPackage::PACK_STX) 
        {
            THROW(CServerException, ERR_PACKET_FMT, "buf[0] == 0x%02x, not STX", _process_buf[0]);
        }
        
        // ������ȡ������buf����
        TRPC_GET_INT32(_process_total, _process_buf + 1);

        // ȷ�����ȷ�Χ
        if ((_process_total <= 0)
                || (_process_total > (int)sizeof(_process_buf)))
        {
            THROW(CServerException, ERR_PACKET_LENGTH, "req packet len error: %d", _process_total);
        }

        // �Ѵ�������ݰ�����ʼ��5���ֽ�ͷ
        _processed = 5;
        
        ret = RECV_HAVE_MORE;
    }

    trpc_debug_log("recv_head ok, total length: %d", _process_total);

    _is_head = true;
    
    return ret;
}

// �հ��Ļ��������հ�ͷ�����ս�β
int CMiddleApi::_recv()
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

    // ������ϣ����ð�ͷ���
    if (ret == RECV_OK)
    {
        _is_head = false;
    }

    return ret;
}

