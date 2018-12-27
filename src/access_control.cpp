#include    "access_control.h"
#include    "trpc_sys_exception.h"
#include <stdlib.h>

CAccessControl::CAccessControl()
{
}

CAccessControl::~CAccessControl()
{
}

void CAccessControl::_clear()
{
    _access_control.clear();
}


int CAccessControl::get_line(const char * str)
{
    // �׳����쳣�ڻ������д���,�������Ƴ�,ֻ�ᱨ��,������Ӱ��,���ﲻ����
    return _get_line(str);
}

int CAccessControl::_get_line(const char * str)
{
    // �����ļ�����ð�ŷָ�
    static const char * sep = ":";
    
    char *          tmp = NULL; // ������strtok_r�Ĳ���
    char *          tok = NULL;
    
    char t[MAX_CONF_LENGTH_SIZE] = {0};
    std::set<int>   request_set;

    // ����һ����ʱ�ַ���,���޸�str
    strncpy(t, str, sizeof(t) - 1);

    std::string ip;

    // request_type
    tok = strtok_r(t, sep, &tmp);
    if (tok == NULL)
    {
        // û�а�request_type�����ƣ�ֱ�Ӽӵ�ip�м���
        // ��һ��Ӧ�ò��ᵽ����
        ip = t;
    }
    else
    {
        ip = strstrip(tok);
    }

    tok = strtok_r(NULL, sep, &tmp);
    if (tok == NULL)
    {
        // do nothing, request_type set������ǿ�
    }
    else
    {
        // ��ȡrequest_type�б�
        std::vector<std::string> tmp_set;

        split(strstrip(tok), ",", MAX_REQUEST_TYPE_PER_IP,  tmp_set);

        for (std::vector<std::string>::iterator it = tmp_set.begin(); 
                it != tmp_set.end(); ++it)
        {
            trpc_debug_log("ip: %s, tok: %s, key: %s", ip.c_str(), tok, it->c_str());
            request_set.insert(atoi(it->c_str()));
        }
    }

    // ������ǰ��set
    _access_control[ip] = request_set;
    
    return 0;
}


void CAccessControl::check(const char * ip, const int request_type)
{
    CAutoRLock l(_mutex);

    _check(ip, request_type);
}

void CAccessControl::check(const char * ip)
{
    CAutoRLock l(_mutex);

    _check(ip);
}

void CAccessControl::_check(const char * ip, const int request_type)
{
    if (_access_control.count(ip) <= 0)
    {
        // ip��������� --  ������Ϊ10003 
        THROW(CServerException, InvalidAccess, "ip: %s not allowed to access", ip);
    }

    // ��������request_type�����ƣ�ֱ�������
    if (_access_control[ip].size() != 0)
    {
        // ����Ƿ�������ʶ�Ӧ��request_type
        if (_access_control[ip].find(request_type) == _access_control[ip].end())
        {
            // ip + request_type ���������
            THROW(CServerException, InvalidAccess, "ip: %s, request_type: %d not allowed to access", ip, request_type);
        }
    }
}

void CAccessControl::_check(const char * ip)
{
    if (_access_control.count(ip) <= 0)
    {
        // ip��������� --  ������Ϊ10003 
        THROW(CServerException, InvalidAccess, "ip: %s not allowed to access", ip);
    }
}
    

