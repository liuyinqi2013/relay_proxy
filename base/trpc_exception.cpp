#include    <sstream>

#include    "trpc_exception.h"

/**
 * Description: 异常类的构造函数
 * Input:  errNum       错误码
 *            szErrInfo     错误描述信息
 */
CTrpcException::CTrpcException(int err_no, const std::string& errinfo, const char* file, const int& line) 
                       : _errno(err_no), _errinfo(errinfo), _file(file), _line(line)
{
}

/**
 * Description: 异常类的析构函数
 */
CTrpcException::~CTrpcException() throw ()
{
}

/**
 * Description: 获取异常的错误码
 * Return: 错误码
 */
int CTrpcException::error()
{
    return _errno;
}

/**
 * Description: 取异常错误描述信息
 * Return: 错误描述信息
 */
const char* CTrpcException::what() 
{
    return _errinfo.c_str();
}

/**
 * Description: 取异常错误描述信息长度
 * Return: 错误描述信息长度
 */
const int CTrpcException::len_what()
{
    return _errinfo.length();
}

/**
 * Description: 取错误代码行号
 * Return: 取错误代码行号
 */
int CTrpcException::line()
{
    return _line;
}

/**
 * Description: 获取错误代码文件名
 * Return: 获取错误代码文件名
 */
const char* CTrpcException::file()
{
    return _file.c_str();
}


const char * CTrpcException::error_info()
{
    std::stringstream ss;

    ss
        << "[" << error() << "][" << file() << "][" 
        << line() << "][" << what() << "]";

    return ss.str().c_str();
}


