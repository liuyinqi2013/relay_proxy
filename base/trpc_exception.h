#ifndef     __TRPC_EXCEPTION_H__
#define     __TRPC_EXCEPTION_H__

#include    <exception>
#include    <string>

#include    <errno.h>
#include    <string.h>
#include    <assert.h>

#include    "trpc_internal_error.h"

extern int errno;

static const int EXCEPTION_MSG_LEN = 4096;

class CTrpcException : public std::exception
{
public:
    CTrpcException(int err_no, const std::string& errinfo, const char* file, const int& line);
    virtual ~CTrpcException() throw (); 

    int error(); // 获取错误码
    int line();  // 取错误代码行号
    const char* file(); // 获取错误代码文件
    virtual const char* what();   // 获取错误描述信息
    virtual const char* error_info();           // 获取错误信息
    const int len_what();
    
protected:
    int _errno;
    std::string _errinfo;
    std::string _file;
    int _line;
};

#define THROW(exception_class, errno, ...) {\
            char __exception_msg[EXCEPTION_MSG_LEN]; \
            snprintf(__exception_msg, EXCEPTION_MSG_LEN - 1, __VA_ARGS__ );\
            throw exception_class(errno, __exception_msg, __FILE__, __LINE__);}

// 其他的派生类的定义
// 不需要做其他实现,只是为了区分错误类型
#define DEFINE_EXCEPTION(COtherException) \
    class COtherException : public CTrpcException \
    { \
    public: \
        COtherException(int err_no, const std::string& errinfo, const char* file, const int& line) \
            : \
            CTrpcException(err_no, errinfo, file, line)\
        { \
        } \
        virtual ~COtherException() throw (){}; \
    };


#endif

