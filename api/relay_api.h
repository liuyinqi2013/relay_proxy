#ifndef     __RELAY_API_H__
#define     __RELAY_API_H__

#include    "api_base.h"

class CRelayApi: public CApiBase
{
public:
    CRelayApi(const char *idc, const char * ip, const int port, const int index, const int time_out);
    virtual ~CRelayApi();

protected:
    // 链接登录等处理
    virtual int _init();

    // 关闭，注销等的处理
    virtual void _fini(bool force_close);
    
    // 编码和解码外部调用, odata和olen为编码后的包，采用外部的buf来控制
    virtual void _encode(const char * packet, const int packet_len, const void * other_info);
    virtual void _decode(char * packet, int & packet_len);

    // recv需要收下包头
    virtual int  _recv();

protected:
    int     _recv_head();

protected:
    bool    _is_connecting;
    bool    _is_head;
};

#endif

