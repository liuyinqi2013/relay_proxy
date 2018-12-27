#ifndef     __TRPC_SYS_EXCEPTION_H__
#define     __TRPC_SYS_EXCEPTION_H__

#include    "trpc_exception.h"
#include    "log.h"
#include    "relay_error.h"

// socket 异常
DEFINE_EXCEPTION(CSocketException);

// ipc 异常
DEFINE_EXCEPTION(CIpcException);

// thread 相关异常
DEFINE_EXCEPTION(CThreadException);

// 配置异常
DEFINE_EXCEPTION(CConfigException);

// 服务相关异常
DEFINE_EXCEPTION(CServerException);


#endif


