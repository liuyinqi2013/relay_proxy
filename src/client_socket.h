#ifndef     __CLIENT_SOCKET_H__
#define     __CLIENT_SOCKET_H__

#include    <time.h>
#include    <vector>
#include    <string>

#include    "hasite_conf.h"
#include    "request_type_conf.h"
#include    "api_base.h"
#include    "socket_base.h"
#include    "system_conf.h"

// �������ӵ�relay��socket���󣬰�����Ҫֱ��ת������������Լ��յ��ķ��ذ���
class CClientSocket: public CSocketBase
{
public:
    CClientSocket();
    ~CClientSocket();

    // ��������
    int     process();

    // ��ʼ��fd -- ������relay��ָ�룬�������������ݵĹ�����ϵ
    void    init(int fd, int thread_no, unsigned int seq_no, void * relay);

    void    fini();
    
    void    decode(const std::vector<CSystemConf::ST_RequestKeyConf>& request_type_array,
                   int relay_server_no);
    void    encode();

public:
    int    get_call_result() const;

    std::string acc_res_packet() const;

    const char * ip()
    {
        return _ip.c_str();
    }

    const int  port()
    {
        return _port;
    }

    const int fd()
    {
        return _fd;
    }

    unsigned int seq_no()
    {
        return _seq_no;
    }

    const int server_fd()
    {
        return _api->fd();
    }

    const int thread_no()
    {
        return _thread_no;
    }

    const int status()
    {
        return _status;
    }

    const char * request_key()
    {
        return _request_key.c_str();
    }

    CRequestTypeConf::ST_RequestConfData& request_conf()
    {
        return _request_conf;
    }

    void api(CApiBase * api)
    {
        //assert(api != NULL);
        
        _api = api;
    }

    CApiBase * api()
    {
        return _api;
    }

    const char * req_packet()
    {
        return _req_packet;
    }

    const int  req_packet_len()
    {
        return _req_packet_len;
    }

    // ������������������api���÷��ذ���
    char * res_packet()
    {
        return _res_packet;
    }

    int& res_packet_len()
    {
        return _res_packet_len;
    }

    void * relay()
    {
        return _relay;
    }

    const int request_key_index()
    {
        return _request_key_index;
    }

    const int request_type()
    {
        return _request_type;
    }

    int  call_middle_microseconds() const;

    int  total_process_microseconds() const;

    void server_ip(const char *server_ip);
    const char *server_ip() const;

    void server_port(int server_port);
    int  server_port() const;

    void set_system_error()
    {
        _is_system_error = true;
    }

    bool is_system_error() const
    {
        return _is_system_error;
    }

    void set_process_time_stamp();

    void set_recv_time_stamp();
    
protected:
    void    _reset();

    int     _recv_head();

    // ����request_key
    void    _gen_request_key(const std::vector<CSystemConf::ST_RequestKeyConf>& request_type_array);

    void    encode_protocol_text();
    void    encode_protocol_binary();

public:
    // ״̬��Ϣ
    enum EM_Status
    {
        EM_FREE = 0,    // ����״̬
        EM_READ = 1,    // �����������
        EM_PROCESS = 2, // ת�������еȴ��Է�����
        EM_SEND = 3,    // ����������Ϣ
        EM_OVER = 4,    // �������
        EM_TIME_OUT = 5,     // ����
    };

protected:
    int     _status;    // ״̬

    // ����������thread_no
    int     _thread_no;

    bool    _is_head;

    // socket��ˮ�ţ����⸴��
    unsigned int _seq_no;

    // �����������
    char    _req_packet[CApiBase::MAX_PACKET_LEN];
    int     _req_packet_len;

    char    _res_packet[CApiBase::MAX_PACKET_LEN];
    int     _res_packet_len;

    int             _request_type;
    
    // ����request_keyƴ�ӵĴ�
    std::string     _request_key;
    int             _request_key_index;

    // ��Ӧ��request_type��Ϣ
    CRequestTypeConf::ST_RequestConfData _request_conf;

    // �����socket��api�ĵ�ַ
    CApiBase * _api;

    void * _relay;
    
    // �������ӹ�����ʱ���
    struct timeval _access_stamp;
    // �������ʱ��
    struct timeval _recv_ok_stamp;
    // ����������ʱ���
    struct timeval _process_stamp;

    bool   _is_system_error;

    // �Է�ip�Ͷ˿�
    std::string _ip;
    int         _port;

    std::string _server_ip;
    int         _server_port;
};

#endif
