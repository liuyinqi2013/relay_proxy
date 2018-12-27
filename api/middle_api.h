#ifndef     __MIDDLE_API_H__
#define     __MIDDLE_API_H__

#include    "api_base.h"

class CMiddleApi: public CApiBase
{
public:
    CMiddleApi(const char *idc, const char * ip, const int port, const std::string& user, const std::string& passwd,
                    const int index, const int time_out);
    virtual ~CMiddleApi();

protected:
    // ����ͽ����ⲿ����, odata��olenΪ�����İ��������ⲿ��buf������
    virtual void _encode(const char * packet, const int packet_len, const void * other_info);
    virtual void _decode(char * packet, int& packet_len);
    
    // ���ӵ�¼�ȴ���
    virtual int _init();

    // �رգ�ע���ȵĴ���
    virtual void _fini(bool force_close);

    // send ����Ҫʵ�֣�recv��Ҫ���°�ͷ
    virtual int  _recv();

protected:
    // ��¼�������
    void    _encode_login_packet();
    void    _decode_login_packet();
    
    int     _recv_head();

protected:
    // ��ʼ��״̬
    enum EM_LOGIN_STATUS
    {
        EM_NO_CONNECT = 1,  // δ����
        EM_CONNECTING = 2,  // ����������
        EM_SENDING = 3,     // ����¼����
        EM_RECVING = 4,     // �յ�¼����
        EM_LOGIN_OK = 5,    // 
    }; 

protected:
    int     _login_status;  // ��¼״̬

    bool    _is_head;
    
    int     _seq_no;
    std::string _user;      // ����middle���û���
    std::string _passwd;    // ����middle������
};

#endif

