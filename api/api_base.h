#ifndef     __API_BASE_H__
#define     __API_BASE_H__

#include    <string>

#include    "socket_base.h"


// api 基类
class CApiBase: public CSocketBase
{
public:
    CApiBase(const char *idc, const char * ip, const int port, const int index, const int time_out);
    virtual ~CApiBase();

    // 处理类，内部有自己的状态机
    // 返回值和client_socket的保持一致
    int     process();

    // 调用这个传进来的指针一定不能是临时变量
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
    // 只能被子类调用
    // 设置为初始化中
    void    _set_initing()
    {
        // connect或者登录中，设置为此状态
        _status = EM_INITING;
    }

protected:
    // 链接登录等处理
    virtual int _init() = 0;

    // 关闭，注销等的处理
    virtual void _fini(bool force_close) = 0; 

    virtual void _encode(const char * packet, const int packet_len, const void * other_info) = 0;
    virtual void _decode(char * packet, int& packet_len) = 0;
    
public:
    enum EM_API_STATUS
    {
        EM_FREE = 0,    // 空闲状态，没有初始化的情况
        EM_CONNECT = 1, // 链接中
        EM_SEND = 2,    // 发送请求信息
        EM_READ = 3,    // 接收请求包中
        EM_TIME_OUT = 4,     // 出错
        EM_CLOSED = 5,  // 待关闭
        EM_OVER = 6,    // 处理完毕，再次调用的时候不需要初始化，可以直接调用
        EM_INITING = 7, // 初始化中，需要持续调用init
        EM_READ_OK = 8, // 接收完毕，等待解码
        EM_READY = 9,   // 编码完毕，已经可以发包
    };

protected:
    int         _status;
    const int   _index;
    // 初始化成功，表示连接也成功，都在init里面做
    bool        _is_init;
    bool        _is_used;

    const char * _req_packet;
    int         _req_len;
    // 保存其他调用信息
    const void * _other_info;

    unsigned int _client_seq_no;
   
    std::string     _idc;
    std::string     _ip;
    int             _port;
    int             _time_out;
};


#endif

