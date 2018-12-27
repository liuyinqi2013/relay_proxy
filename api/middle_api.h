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
    // 编码和解码外部调用, odata和olen为编码后的包，采用外部的buf来控制
    virtual void _encode(const char * packet, const int packet_len, const void * other_info);
    virtual void _decode(char * packet, int& packet_len);
    
    // 连接登录等处理
    virtual int _init();

    // 关闭，注销等的处理
    virtual void _fini(bool force_close);

    // send 不需要实现，recv需要收下包头
    virtual int  _recv();

protected:
    // 登录包编解码
    void    _encode_login_packet();
    void    _decode_login_packet();
    
    int     _recv_head();

protected:
    // 初始化状态
    enum EM_LOGIN_STATUS
    {
        EM_NO_CONNECT = 1,  // 未连接
        EM_CONNECTING = 2,  // 正在连接中
        EM_SENDING = 3,     // 发登录包中
        EM_RECVING = 4,     // 收登录包中
        EM_LOGIN_OK = 5,    // 
    }; 

protected:
    int     _login_status;  // 登录状态

    bool    _is_head;
    
    int     _seq_no;
    std::string _user;      // 链接middle的用户名
    std::string _passwd;    // 链接middle的密码
};

#endif

