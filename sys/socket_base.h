#ifndef     __SOCKET_BASE_H__
#define     __SOCKET_BASE_H__

// ��װ�첽��send��recv
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
    //  �������󳤶� -- ����һ�������32k�����淢�͵�ʱ��
    static const int MAX_PACKET_LEN = 32000;

    // ����ķ���ֵ
    static const int RECV_OK        = 1;    // �յ�һ��������trpc��
    static const int RECV_HAVE_MORE = 2;    // ����û������

    static const int SEND_OK        = 5;    // ���ѷ���
    static const int SEND_HAVE_MORE = 6;    // ����û����

    static const int CONNECTING     = 7;    // ������
    static const int INIT_OK        = 8;    // ���ӳɹ�

protected:
    // ֻ����
    int     _fd;
    
    // �����еĻ�����������read����write��ʾ��ͬ����Ϣ
    char    _process_buf[CSocketBase::MAX_PACKET_LEN];
    int     _process_total;     // ��Ҫ������ܳ���
    int     _processed;         // �Ѵ�����ܳ���
};

#endif

