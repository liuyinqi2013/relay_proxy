#ifndef     __TRPC_INTERNAL_ERROR_H__
#define     __TRPC_INTERNAL_ERROR_H__

#define     SVR_SYSTEM_ID   "relay"

#include    "relay_error.h"

// ���������relay�ľɴ������Ӧ
enum em_BASE_ERROR_NO
{
    SUCCESS                 = 0,

    /*
    *   ϵͳ�쳣
    */
    ERR_SYS_UNKNOWN         = SysError,     // �����쳣

    /*
    *   ���ô���
    */ 
    ERR_CONF                = SysError,     // ���ô���
    
    /*
    *   socket�쳣
    */ 
    ERR_SOCKET_AGAIN        = SocketError,     // EAGAIN��Ĵ�����
    ERR_SOCKET_TERMINAL     = SocketError,     // ��Ҫֹͣ�Ĵ�����

    ERR_SOCKET_TIME_OUT     = CoreCenterError,     // ��ʱ
    ERR_SOCKET_CONN_REFUSED = CoreCenterError,     // ���ӱ��ܾ� -- �����������������
    ERR_SOCKET_RECV_CLOSE   = RecvClose,     // ����ر�
    
    /*
    *   IPC�쳣
    */
    ERR_IPC_ERROR           = SocketError,
    ERR_PACKET_LENGTH       = SocketError,     // �����ȴ���
    ERR_PACKET_FMT          = SocketError,     // ���ṹ����
    ERR_PACKET_UNKNOW       = SocketError,     // ����������
    ERR_PACKET_ERROR        = SocketError,
    ERR_PACKET_OLD          = SystemBusy,     // ��̫��, ���������Ѿ���ʱ����

    /*
    *   ������ص�ϵͳ�쳣
    */
    ERR_SERVER_ERROR        = SystemBusy,     // ����δ�������
    ERR_SERVER_MAINTAIN     = SystemBusy,     // ��̨middleϵͳά��
    ERR_SERVER_BUSY         = SystemBusy,     // ��̨��æû�п��ýڵ�
    
    /*
    *   ������������쳣
    */
    ERR_ADMIN_CMD           = AdminCmdError,     // �����������     
};

#define     SUCCESS_INFO    "ok"

#endif

