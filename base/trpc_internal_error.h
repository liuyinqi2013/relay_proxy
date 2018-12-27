#ifndef     __TRPC_INTERNAL_ERROR_H__
#define     __TRPC_INTERNAL_ERROR_H__

#define     SVR_SYSTEM_ID   "relay"

#include    "relay_error.h"

// 将错误码和relay的旧错误码对应
enum em_BASE_ERROR_NO
{
    SUCCESS                 = 0,

    /*
    *   系统异常
    */
    ERR_SYS_UNKNOWN         = SysError,     // 不明异常

    /*
    *   配置错误
    */ 
    ERR_CONF                = SysError,     // 配置错误
    
    /*
    *   socket异常
    */ 
    ERR_SOCKET_AGAIN        = SocketError,     // EAGAIN类的错误码
    ERR_SOCKET_TERMINAL     = SocketError,     // 需要停止的错误码

    ERR_SOCKET_TIME_OUT     = CoreCenterError,     // 超时
    ERR_SOCKET_CONN_REFUSED = CoreCenterError,     // 链接被拒绝 -- 单独处理这个错误码
    ERR_SOCKET_RECV_CLOSE   = RecvClose,     // 对面关闭
    
    /*
    *   IPC异常
    */
    ERR_IPC_ERROR           = SocketError,
    ERR_PACKET_LENGTH       = SocketError,     // 包长度错误
    ERR_PACKET_FMT          = SocketError,     // 包结构错误
    ERR_PACKET_UNKNOW       = SocketError,     // 其他包错误
    ERR_PACKET_ERROR        = SocketError,
    ERR_PACKET_OLD          = SystemBusy,     // 包太老, 可能外面已经超时返回

    /*
    *   服务相关的系统异常
    */
    ERR_SERVER_ERROR        = SystemBusy,     // 服务未定义错误
    ERR_SERVER_MAINTAIN     = SystemBusy,     // 后台middle系统维护
    ERR_SERVER_BUSY         = SystemBusy,     // 后台繁忙没有可用节点
    
    /*
    *   管理命令相关异常
    */
    ERR_ADMIN_CMD           = AdminCmdError,     // 管理命令错误     
};

#define     SUCCESS_INFO    "ok"

#endif

