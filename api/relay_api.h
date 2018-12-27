#ifndef     __RELAY_API_H__
#define     __RELAY_API_H__

#include    "api_base.h"

class CRelayApi: public CApiBase
{
public:
    CRelayApi(const char *idc, const char * ip, const int port, const int index, const int time_out);
    virtual ~CRelayApi();

protected:
    // ���ӵ�¼�ȴ���
    virtual int _init();

    // �رգ�ע���ȵĴ���
    virtual void _fini(bool force_close);
    
    // ����ͽ����ⲿ����, odata��olenΪ�����İ��������ⲿ��buf������
    virtual void _encode(const char * packet, const int packet_len, const void * other_info);
    virtual void _decode(char * packet, int & packet_len);

    // recv��Ҫ���°�ͷ
    virtual int  _recv();

protected:
    int     _recv_head();

protected:
    bool    _is_connecting;
    bool    _is_head;
};

#endif

