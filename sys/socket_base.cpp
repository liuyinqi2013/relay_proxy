#include    "socket_base.h"
#include    "trpc_socket_opt.h"
#include    "trpc_sys_exception.h"

CSocketBase::CSocketBase()
    :
    _fd(INVALID_SOCKET),
    _process_total(0),
    _processed(0)
{
}

CSocketBase::~CSocketBase()
{
    CSocketOpt::close(_fd);
}

int CSocketBase::_recv()
{
    //DEBUG_TRACE();
    
    int ret = -1;
    int size = -1;

    trpc_debug_log("read fd: %d,  _process_total: %d - _processed: %d", _fd, _process_total, _processed);
    
    size = CSocketOpt::read(_fd, _process_buf + _processed, _process_total - _processed);

    if (size < 0)
    {
        return RECV_HAVE_MORE;
    }

    // 累加长度，< 0 或者 == 0的情况read里面已经处理
    _processed += size;

    if (_processed < _process_total)
    {
        // 继续读取
        ret = RECV_HAVE_MORE;
    }
    else if (_processed > _process_total)
    {
        // 应该不会出现，应该是编码问题
        THROW(CTrpcException, ERR_SYS_UNKNOWN, "processed: %d > _process_total: %d", _processed, _process_total);
    }
    else
    {
        // 处理完毕
        ret = RECV_OK;
    }

    return ret;
}

int CSocketBase::_send()
{
    int ret = -1;
    int size = -1;
    
    trpc_debug_log("write fd: %d, _process_total: %d - _processed: %d", _fd, _process_total, _processed);
    
    size = CSocketOpt::write(_fd, _process_buf + _processed, _process_total - _processed);

    if (size < 0)
    {
        return SEND_HAVE_MORE;
    }

    // 累加长度，< 0 或者 == 0的情况read里面已经处理
    _processed += size;

    if (_processed < _process_total)
    {
        // 继续写
        ret = SEND_HAVE_MORE;
    }
    else if (_processed > _process_total)
    {
        // 应该不会出现，应该是编码问题
        THROW(CTrpcException, ERR_SYS_UNKNOWN, "processed: %d > _process_total: %d", _processed, _process_total);
    }
    else
    {
        // 处理完毕
        ret = SEND_OK;
    }

    return ret;
}

