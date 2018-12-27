#ifndef     __REQUEST_TYPE_CONF_H__
#define     __REQUEST_TYPE_CONF_H__

#include    <string>
#include    <vector>
#include    <map>

#include    "thread_mutex.h"
#include    "trpc_config.h"

enum EM_Property
{
    EM_PROPERTY_DISPATCH = 1,   // ֱ��ת��
    EM_PROPERTY_PROCESS  = 2,   // ��ֱ��ת��  
    EM_PROPERTY_MAX,            // �������ֵ,У��Ϸ�����
};

enum EM_Protocol
{
    EM_PROTOCOL_TEXT    = 1,    // �ı�Э��
    EM_PROTOCOL_BIN     = 2,    // ������Э��
    EM_PROTOCOL_MAX,            // �������ֵ,У��Ϸ�����
};

class CRequestTypeConf: public CConfig
{
public:
    CRequestTypeConf();
    virtual ~CRequestTypeConf();

    void  dump();

protected:
    virtual int get_line(const char * str);
    virtual void _clear();
    
    int     _get_line(const char * str);
    
public:
    // request_type ��ȡ�����÷�ʽΪ
    // �ؼ���1^�ؼ���2^����Ϊkey,Ȼ��ƴ��·��
    static const char * REQUEST_KEY_SPLIT_CHAR;
    
    // �����ļ�����ð�ŷָ�
    static const char * REQUEST_SPLIT_CHAR;
    
    // ����request_type key�ĳ��� 128λӦ���㹻,һ����Ƿ�������+��������
    static const int MAX_REQUEST_TYPE_LEN = 64;
    
    // ���service_name
    static const int MAX_SERVICE_NAME = 64;

public:
    struct ST_RequestConfData
    {
        char    request_type[MAX_REQUEST_TYPE_LEN];
        int     property;
        int     site_no;
        int     protocol;
        char    service_name[MAX_SERVICE_NAME];
        // �����ľ�relay����Ϣ�Ѳ���Ҫ��ʹ��,������ʵҲ���ٷ���ͳ����Ϣ
    };

public:
    // ����request_type��Ϣ,��ȡ����
    void get(const char * request_type, ST_RequestConfData& request_conf);

protected:
    std::map<std::string, ST_RequestConfData>   _request_conf;
};

#endif

