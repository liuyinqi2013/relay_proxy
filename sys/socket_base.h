#ifndef     __SOCKET_BASE_H__
#define     __SOCKET_BASE_H__

// 封装异步的send和recv
class CSocketBase
{
public:
    CSocketBase();
    virtual ~CSocketBase();
    
    const int fd()
    {
        return _fd;
    }

protected:
    virtual int     _recv();
    virtual int     _send();

public:
    //  请求包最大长度 -- 现在一般最大都是32k，后面发送的时候
    static const int MAX_PACKET_LEN = 32000;

    // 处理的返回值
    static const int RECV_OK        = 1;    // 收到一个完整的trpc包
    static const int RECV_HAVE_MORE = 2;    // 包还没有收完

    static const int SEND_OK        = 5;    // 包已发完
    static const int SEND_HAVE_MORE = 6;    // 包还没发完

    static const int CONNECTING     = 7;    // 连接中
    static const int INIT_OK        = 8;    // 链接成功

protected:
    // 只处理
    int     _fd;
    
    // 处理中的缓冲区，根据read或者write表示不同的信息
    char    _process_buf[CSocketBase::MAX_PACKET_LEN];
    int     _process_total;     // 需要处理的总长度
    int     _processed;         // 已处理的总长度
};

#endif

