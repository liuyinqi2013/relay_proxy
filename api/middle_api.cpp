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
    
    // 将请求包拼接
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

    // 解码
    if (outPack.Decode(_process_buf, _process_total) != 0)
    {
        // 解码出错
        THROW(CServerException, ERR_PACKET_ERROR, "Decode: %s", outPack.get_error_text());
    }

    // 检查返回包结果
    if (outPack.GetMsgType() != CTrpcPackage::TYPE_CALL_RESPOND) 
    {
        THROW(CServerException, ERR_PACKET_ERROR, "Respond MsgType(%d) != TYPE_LOGIN_RESPOND",
                outPack.GetMsgType());
    }

    if (outPack.GetResult() != 0) 
    {
        // 执行失败
        THROW(CServerException, ERR_SERVER_ERROR, "Login error: %s",
                outPack.GetData());
    }

    packet_len = outPack.GetDataLen();
    memcpy(packet, outPack.GetData(), packet_len);
}

// 链接登录等处理
int CMiddleApi::_init()
{
    int ret = -1;

    // 先检查是否已经开始连接
    switch(_login_status)
    {
        case EM_NO_CONNECT:
        {      
            assert(_fd == INVALID_SOCKET);
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

            _login_status = EM_CONNECTING;

            break;
        }
        case EM_CONNECTING:
        {
            // 为connect后的第二次触发，检查connect是否成功
            CSocketOpt::check_connect_status(_fd);

            // 编码登录包
            _encode_login_packet();
            
            // 发送登录包
            _login_status = EM_SENDING;
            
            ret = SEND_HAVE_MORE;
            
            // 链接成功以后，继续发包，不做break
        }
        case EM_SENDING:
        {
            // 链接成功，发送登陆包
            ret = _send();

            if (ret != SEND_OK)
            {
                break;
            }

            // 已经发送完毕，清空buf
            _processed = 0;
            _process_total = 0;

            _login_status = EM_RECVING;

            // 等有请求再做接收
            break;
        }
        case EM_RECVING:
        {
            ret = _recv();

            if (ret != RECV_OK)
            {
                break;
            }

            // 解包
            _decode_login_packet();

            trpc_debug_log("login ok: %s: %d, user: %s, pass: %s", ip(), port(), _user.c_str(), _passwd.c_str());

            // 到这里的话，就是登录正常了

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

// 关闭，注销等的处理
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

// 登录包编解码
void CMiddleApi::_encode_login_packet()
{
    // 组装登录包
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

    // 解码
    if (outPack.Decode(_process_buf, _process_total) != 0)
    {
        // 解码出错
        THROW(CServerException, ERR_PACKET_ERROR, "Decode: %s", outPack.get_error_text());
    }

    // 检查返回包结果
    if (outPack.GetMsgType() != CTrpcPackage::TYPE_LOGIN_RESPOND) 
    {
        THROW(CServerException, ERR_PACKET_ERROR, "Respond MsgType(%d) != TYPE_LOGIN_RESPOND",
                outPack.GetMsgType());
    }

    if (outPack.GetResult() != 0) 
    {
        // 执行失败
        THROW(CServerException, ERR_SERVER_ERROR, "Login error: %s",
                outPack.GetData());
    }

    // 登录成功
    _login_status = EM_LOGIN_OK;
}

int CMiddleApi::_recv_head()
{
    DEBUG_TRACE();
    
    int ret = -1;
    
    // 可以读，先读5个字节长度
    int size = -1;
    size = CSocketOpt::read(_fd, _process_buf, 5);

    if (size < 5)
    {
        // 应该不可能出现5个字节都收不完的情况，直接报错
        THROW(CSocketException, ERR_IPC_ERROR, "read size error: %d < 5", size);
    }
    else
    {
        // 检查格式
        if (_process_buf[0] != CTrpcPackage::PACK_STX) 
        {
            THROW(CServerException, ERR_PACKET_FMT, "buf[0] == 0x%02x, not STX", _process_buf[0]);
        }
        
        // 正常读取，设置buf长度
        TRPC_GET_INT32(_process_total, _process_buf + 1);

        // 确定长度范围
        if ((_process_total <= 0)
                || (_process_total > (int)sizeof(_process_buf)))
        {
            THROW(CServerException, ERR_PACKET_LENGTH, "req packet len error: %d", _process_total);
        }

        // 已处理的数据包含开始的5个字节头
        _processed = 5;
        
        ret = RECV_HAVE_MORE;
    }

    trpc_debug_log("recv_head ok, total length: %d", _process_total);

    _is_head = true;
    
    return ret;
}

// 收包的话都是先收包头，再收结尾
int CMiddleApi::_recv()
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

    // 接收完毕，重置包头标记
    if (ret == RECV_OK)
    {
        _is_head = false;
    }

    return ret;
}

