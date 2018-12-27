#ifndef     __API_BASE_H__
#define     __API_BASE_H__

#include    <string>

#include    "socket_base.h"


// api ����
class CApiBase: public CSocketBase
{
public:
    CApiBase(const char *idc, const char * ip, const int port, const int index, const int time_out);
    virtual ~CApiBase();

    // �����࣬�ڲ����Լ���״̬��
    // ����ֵ��client_socket�ı���һ��
    int     process();

    // ���������������ָ��һ����������ʱ����
    void    encode(const char * packet, const int packet_len, const void * other_info);
    void    decode(char * packet, int & packet_len);

    int     init();
    void    fini(bool force_close);

    const int index();

    bool    is_free();
    void    set_free(bool force_close);
    void    set_used();

    void    client_seq_no(unsigned int seq_no)
    {
        _client_seq_no = seq_no;
    }

    unsigned int client_seq_no() const
    {
        return _client_seq_no;
    }

    const char *idc() const
    {
        return _idc.c_str();
    }

    const char * ip()
    {
        return _ip.c_str();
    }

    const int port()
    {
        return _port;
    }

    const int time_out()
    {
        return _time_out;
    }

protected:
    // ֻ�ܱ��������
    // ����Ϊ��ʼ����
    void    _set_initing()
    {
        // connect���ߵ�¼�У�����Ϊ��״̬
        _status = EM_INITING;
    }

protected:
    // ���ӵ�¼�ȴ���
    virtual int _init() = 0;

    // �رգ�ע���ȵĴ���
    virtual void _fini(bool force_close) = 0; 

    virtual void _encode(const char * packet, const int packet_len, const void * other_info) = 0;
    virtual void _decode(char * packet, int& packet_len) = 0;
    
public:
    enum EM_API_STATUS
    {
        EM_FREE = 0,    // ����״̬��û�г�ʼ�������
        EM_CONNECT = 1, // ������
        EM_SEND = 2,    // ����������Ϣ
        EM_READ = 3,    // �����������
        EM_TIME_OUT = 4,     // ����
        EM_CLOSED = 5,  // ���ر�
        EM_OVER = 6,    // ������ϣ��ٴε��õ�ʱ����Ҫ��ʼ��������ֱ�ӵ���
        EM_INITING = 7, // ��ʼ���У���Ҫ��������init
        EM_READ_OK = 8, // ������ϣ��ȴ�����
        EM_READY = 9,   // ������ϣ��Ѿ����Է���
    };

protected:
    int         _status;
    const int   _index;
    // ��ʼ���ɹ�����ʾ����Ҳ�ɹ�������init������
    bool        _is_init;
    bool        _is_used;

    const char * _req_packet;
    int         _req_len;
    // ��������������Ϣ
    const void * _other_info;

    unsigned int _client_seq_no;
   
    std::string     _idc;
    std::string     _ip;
    int             _port;
    int             _time_out;
};


#endif

