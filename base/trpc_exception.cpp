#include    <sstream>

#include    "trpc_exception.h"

/**
 * Description: �쳣��Ĺ��캯��
 * Input:  errNum       ������
 *            szErrInfo     ����������Ϣ
 */
CTrpcException::CTrpcException(int err_no, const std::string& errinfo, const char* file, const int& line) 
                       : _errno(err_no), _errinfo(errinfo), _file(file), _line(line)
{
}

/**
 * Description: �쳣�����������
 */
CTrpcException::~CTrpcException() throw ()
{
}

/**
 * Description: ��ȡ�쳣�Ĵ�����
 * Return: ������
 */
int CTrpcException::error()
{
    return _errno;
}

/**
 * Description: ȡ�쳣����������Ϣ
 * Return: ����������Ϣ
 */
const char* CTrpcException::what() 
{
    return _errinfo.c_str();
}

/**
 * Description: ȡ�쳣����������Ϣ����
 * Return: ����������Ϣ����
 */
const int CTrpcException::len_what()
{
    return _errinfo.length();
}

/**
 * Description: ȡ��������к�
 * Return: ȡ��������к�
 */
int CTrpcException::line()
{
    return _line;
}

/**
 * Description: ��ȡ��������ļ���
 * Return: ��ȡ��������ļ���
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


