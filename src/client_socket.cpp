#include    <sys/time.h>

#include    "client_socket.h"
#include    "trpc_socket_opt.h"
#include    "trpc_url_param_map.h"
#include    "request_type_conf.h"
#include    "errcode_escape_conf.h"
#include    "detail_info_service_conf.h"

CClientSocket::CClientSocket()
    :
    CSocketBase()
{
    _reset();
}

CClientSocket::~CClientSocket()
{
   CSocketOpt::close(_fd);
}

int CClientSocket::process()
{
    DEBUG_TRACE();
    
    int ret = -1;
    
    switch(_status)
    {
        case EM_READ:
            DEBUG_TRACE();
            if (!_is_head)
            {
                // 如果没有收包头，先收包头
                ret = _recv_head();
            }

            ret = _recv();
            trpc_debug_log("after recv length: %d, left len: %d, buf is: [%s]",
                            _process_total, _processed, _process_buf + sizeof(int));
            if (ret == RECV_OK)
            {
                // 修改状态为处理中
                _status = EM_PROCESS;

                // 设置接收时间戳
                gettimeofday(&_recv_ok_stamp, 0);
            }
            break;
        case EM_SEND:
            ret = _send();
            if (ret == SEND_OK)
            {  
                // 修改状态为发送完毕
                _status = EM_OVER;

                gettimeofday(&_process_stamp, 0);
            }
            break;
        default:
            THROW(CTrpcException, ERR_SYS_UNKNOWN, "unknown status: %d", _status);
    }

    return ret;
}

void CClientSocket::init(int fd, int thread_no, unsigned int seq_no, void * relay)
{
    // 确认fd必须为初始化的fd，否则应该为代码异常，挂掉重新拉起
    assert(_fd == INVALID_SOCKET);

    // 清空信息结束时已reset;
    if (_thread_no >= 0)
    {
        // socket已经有在使用，或者线程错乱，先报错
        THROW(CTrpcException, ERR_SYS_UNKNOWN, "fd: %d old thread: %d, new thread: %d",
                    fd, _thread_no, thread_no);
    }

    _fd = fd;

    // 设置fd为非阻塞
    CSocketOpt::set_nonblock(_fd, true);

    // 获取对方ip
    CSocketOpt::get_peer_addr(_fd, _ip, _port);

    // 设置access time
    gettimeofday(&_access_stamp, 0);

    memset(&_recv_ok_stamp, 0x00, sizeof(_recv_ok_stamp));
    memset(&_process_stamp, 0x00, sizeof(_process_stamp));

    // 设置该socket所属线程
    _thread_no = thread_no;

    // accept成功修改状态为读
    _status = EM_READ;
    _seq_no = seq_no;

    _relay = relay;
}

void CClientSocket::fini()
{
    _reset();
}

void CClientSocket::_reset()
{
    // 其他信息清空
    _status = EM_FREE;
    
    memset(_req_packet, 0x00, sizeof(_req_packet));
    _req_packet_len = 0;

    memset(_res_packet, 0x00, sizeof(_res_packet));
    _res_packet_len = 0;

    _request_key.clear();
    
    memset(&_access_stamp, 0x00, sizeof(_access_stamp));
    memset(&_recv_ok_stamp, 0x00, sizeof(_recv_ok_stamp));
    memset(&_process_stamp, 0x00, sizeof(_process_stamp));
    memset(&_request_conf, 0x00, sizeof(_request_conf));

    memset(_process_buf, 0x00, sizeof(_process_buf));
    _process_total = 0;
    _processed = 0;

    _ip.clear();
    _port = 0;

    _thread_no = -1;
    _is_head = false;
    _api = NULL;
    _request_key_index = -1;
    _request_type = -1;

    _server_ip.clear();
    _server_port = 0;

    _is_system_error = false;

    CSocketOpt::close(_fd);
}

int CClientSocket::_recv_head()
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

        // 设置socket状态为开始接收
        _status = EM_READ;
        
        ret = RECV_HAVE_MORE;
    }

    trpc_debug_log("recv_head ok, total length: %d", _process_total);

    _is_head = true;
    
    return ret;
}

void CClientSocket::decode(const std::vector<CSystemConf::ST_RequestKeyConf>& request_type_array,
                           int relay_server_no)
{
    static unsigned int msg_index = 0;

    // 解析buf中的请求包中
    memcpy(&_req_packet_len, _process_buf, sizeof(int));

    memset(_req_packet, 0x00, sizeof(_req_packet));

    // 请求包中没有MSG_NO，并且请求包长度小于 CApiBase::MAX_PACKET_LEN - 64，生成MSG_NO
    if (strstr(_process_buf+sizeof(int), "MSG_NO") == NULL && _process_total <= CApiBase::MAX_PACKET_LEN - 64)
    {
        char msg_no[CApiBase::MAX_PACKET_LEN+1];
        memset(msg_no, 0x00, sizeof(msg_no));
        const int msg_no_len = snprintf(msg_no, sizeof(msg_no)-1, "MSG_NO=%03u%010u%010u&", 
                                        relay_server_no, (unsigned int)time(NULL), ++msg_index);
        memcpy(_req_packet, msg_no, msg_no_len);
        memcpy(_req_packet + msg_no_len, _process_buf + sizeof(int), _req_packet_len);
        _req_packet_len += msg_no_len;
    }
    else
    {
        memcpy(_req_packet, _process_buf + sizeof(int), _req_packet_len);
    }

    _gen_request_key(request_type_array);

    // 解码成功，修改状态为服务处理中
    _status = EM_PROCESS;

    trpc_debug_log("recv ok request_type_array: %s, req_packet: %s", _request_key.c_str(), _req_packet);
}

void CClientSocket::_gen_request_key(const std::vector<CSystemConf::ST_RequestKeyConf>& request_type_array)
{
    if (request_type_array.size() == 0)
    {
        THROW(CTrpcException, ERR_SYS_UNKNOWN, "request_type array is empty");
    }

    _request_key.clear();
    
    // 根据请求包生成request_key
    CTrpcUrlParamMap param(_req_packet);

    // 检查request_type
    if (param.isEmpty(CSystemConf::REQUEST_TYPE_NAME))
    {
        THROW(CServerException, ERR_PACKET_FMT, "key param: %s not found", CSystemConf::REQUEST_TYPE_NAME);
    }

    // 单独记录request_type
    _request_type = atoi(param[CSystemConf::REQUEST_TYPE_NAME].c_str());

    _request_key_index = -1;
    int index = 0;

    // 根据邋request_type的值的范围来确定是否选择
    for (std::vector<CSystemConf::ST_RequestKeyConf>::const_iterator t_key = request_type_array.begin();
          t_key != request_type_array.end(); 
		  ++t_key, ++index)
    {
        if (_request_type >= t_key->begin && _request_type < t_key->end)
        {
            _request_key_index = index;
        }
    }

    if (_request_key_index < 0)
    {
        // 没找到对应的key
        THROW(CServerException, ERR_PACKET_ERROR, "request_type error: %d", _request_type);
    }

    trpc_debug_log("get request_type: %d, key_index: %d", _request_type, _request_key_index);

    std::vector<std::string>::const_iterator it = request_type_array[_request_key_index].request_key.begin();

    bool is_first = true;

    do
    {
        trpc_debug_log("request_type: %s, _req_packet: %s", param["request_type"].c_str(), _req_packet);
        
        if (!is_first)
        {
            _request_key += CRequestTypeConf::REQUEST_KEY_SPLIT_CHAR;
        }
        
        if (param.isEmpty(*it))
        {
            // 参数不存在，或者为空 -- 参数错误
            THROW(CServerException, ERR_PACKET_FMT, "key param: %s not found", it->c_str());
        }
                
        _request_key += param[*it];

        is_first = false;

        ++it;
    }while(it != request_type_array[_request_key_index].request_key.end());
}

void CClientSocket::encode()
{
    if (_request_conf.protocol == EM_PROTOCOL_BIN)
    {
        encode_protocol_binary();
    }
    else
    {
        encode_protocol_text();
    }

    //_process_total = sizeof(int) + _res_packet_len;
    //_processed = 0;

    _status = EM_SEND;
}

void CClientSocket::encode_protocol_text()
{
    CTrpcUrlParamMap param(_res_packet);
   
    int result = atoi(param["result"].c_str());

    std::string escape_text;
    if (result != 0)
    {
        bool is_detail_service = g_detail_info_service_conf.is_detail_info_service(this->request_type());
        CTrpcUrlParamMap res_param;
        if (is_detail_service)
        {
            res_param = param;
        }
        else
        {
            res_param["result"] = param["result"];
            res_param["res_info"] = param["res_info"];

            if (param.count("comm_value") > 0)
            {
                res_param["comm_value"] = param["comm_value"];
            }
        }

        if (g_errcode_escape_conf.isOriginalCode(result))
        {
        }
        else if (g_errcode_escape_conf.isEscapedCode(result))
        {
            escape_text = g_errcode_escape_conf.getEscapedText(result);
        }
        else
        {
            //escape_text = "[" + param["result"] + "]" + "system busy";
            /*modify by hbt for res_info 20140813 begin*/
	    escape_text = param["res_info"]; //"[" + param["result"] + "]" + "system busy";
            /*modify by hbt for res_info 20140813 end*/
        }

        if (escape_text.length() > 0)
        {
            res_param["res_info"] = escape_text;
        }
	else
	{
	    /*modify by hbt for res_info 20140813 begin*/
	    res_param["res_info"] = "[" + param["result"] + "]" + "system busy";
	    /*modify by hbt for res_info 20140813 end*/
	}

        std::string packet;
        res_param.packUrl(packet);
        memset(_process_buf, 0x00, sizeof(_process_buf));
        int len = packet.length();
        memcpy(_process_buf, &len, sizeof(int));
        memcpy(_process_buf + sizeof(int), packet.c_str(), len);

        _process_total = sizeof(int) + len;
        _processed = 0;
    }
    else
    {
        // 将请求包放入buf，准备发送
        memset(_process_buf, 0x00, sizeof(_process_buf));

        memcpy(_process_buf, &_res_packet_len, sizeof(int));
        memcpy(_process_buf + sizeof(int), _res_packet, _res_packet_len);

        _process_total = sizeof(int) + _res_packet_len;
        _processed = 0;
    }
}

void CClientSocket::encode_protocol_binary ()
{
    const int result = *(((int *)_res_packet));

    std::string escape_text;
    if (result != 0)
    {
        if (g_errcode_escape_conf.isOriginalCode(result))
        {
        }
        else if (g_errcode_escape_conf.isEscapedCode(result))
        {
            escape_text = g_errcode_escape_conf.getEscapedText(result);
        }
        else
        {
            escape_text = "[" + to_string(result) + "]" + "system busy";
        }
    }

    memset(_process_buf, 0x00, sizeof(_process_buf));
    if (escape_text.length() > 0)
    {
        _res_packet_len = sizeof(int) + escape_text.length();
        memcpy(_process_buf, &_res_packet_len, sizeof(int));
        memcpy(_process_buf+sizeof(int), &result, sizeof(int));
        memcpy(_process_buf+sizeof(int)*2, escape_text.c_str(), escape_text.length());
    }
    else
    {
        memcpy(_process_buf, &_res_packet_len, sizeof(int));
        memcpy(_process_buf + sizeof(int), _res_packet, _res_packet_len);
    }

    _process_total = sizeof(int) + _res_packet_len;
    _processed = 0;
}


int  
CClientSocket::call_middle_microseconds() const
{
    if (_process_stamp.tv_sec != 0 && _recv_ok_stamp.tv_sec != 0)
    {
        return  (_process_stamp.tv_sec - _recv_ok_stamp.tv_sec) * 1000000 + _process_stamp.tv_usec - _recv_ok_stamp.tv_usec;
    }

    return 0;
}

void 
CClientSocket::server_ip(const char *server_ip)
{
    _server_ip = server_ip;
}

const  char *
CClientSocket::server_ip() const
{
   return _server_ip.c_str();
}

void 
CClientSocket::server_port(int server_port)
{
    _server_port = server_port;
}

int  
CClientSocket::server_port() const
{
    return _server_port;
}

int    
CClientSocket::get_call_result() const
{
    int result = 0;
    if (_request_conf.protocol == EM_PROTOCOL_BIN)
    {
        result = *((int *)(_res_packet));
    }
    else
    {
        CTrpcUrlParamMap param;
        param.parseUrl(_res_packet);
        result = atoi(param["result"].c_str());
    }

    return result;
}

std::string
CClientSocket::acc_res_packet() const
{
    if (_request_conf.protocol == EM_PROTOCOL_BIN)
    {
        int result = get_call_result();
        if (result == 0) 
        {
            return _res_packet;
        }
        else
        {
            char buf[1024];
            snprintf(buf, sizeof(buf)-1, "result=%d&res_info=%s", result, _res_packet+sizeof(int));
            return buf;
        }
    }
    else
    {
        return _res_packet;
    }
}

void 
CClientSocket::set_process_time_stamp()
{
    if (_process_stamp.tv_sec == 0)
    {
        gettimeofday(&_process_stamp, 0);
    }
}

void 
CClientSocket::set_recv_time_stamp()
{
    if (_recv_ok_stamp.tv_sec == 0)
    { 
        gettimeofday(&_recv_ok_stamp, 0);
    }
}

int  
CClientSocket::total_process_microseconds() const
{
    if (_process_stamp.tv_sec != 0 && _access_stamp.tv_sec != 0)
    {
        return (_process_stamp.tv_sec - _access_stamp.tv_sec) * 1000000 + _process_stamp.tv_usec - _access_stamp.tv_usec;
    }

    return 0;
}
