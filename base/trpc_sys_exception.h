#ifndef     __TRPC_SYS_EXCEPTION_H__
#define     __TRPC_SYS_EXCEPTION_H__

#include    "trpc_exception.h"
#include    "log.h"
#include    "relay_error.h"

// socket �쳣
DEFINE_EXCEPTION(CSocketException);

// ipc �쳣
DEFINE_EXCEPTION(CIpcException);

// thread ����쳣
DEFINE_EXCEPTION(CThreadException);

// �����쳣
DEFINE_EXCEPTION(CConfigException);

// ��������쳣
DEFINE_EXCEPTION(CServerException);


#endif


