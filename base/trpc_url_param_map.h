#ifndef __TRPC_URL_PARAM_MAP__
#define __TRPC_URL_PARAM_MAP__

#include <map>
#include <string>
#include <ostream>

using std::map;
using std::string;

/**
 * ����(��)ת�庯��ָ��
 */
typedef string (*PFUN_ESCAPE)(const string&);
typedef string (*PFUN_UNESCAPE)(const string&);

/**
 * ȱʡ�ָ���ת���ַ�
 */
extern const char DFL_SEG_DEL;   
extern const char DFL_ELE_DEL;

/**
* ������ڵ�һλ����Ϣ
*/
extern const char* __MUST_IN_FIRST_FIELD;

/**
 * ����(Name-value)ӳ���
 */
class CTrpcUrlParamMap: public std::map<string,string>
{
public:
    static char __SEG_DEL; // �ηָ���
    static char __ELE_DEL; // ����-ֵ�ָ���
    static PFUN_ESCAPE __PFUN_ESCAPE; // ת�庯��ָ�� 
    static PFUN_UNESCAPE __PFUN_UNESCAPE; // ת�庯��ָ�� 
    static string encodeUrl(const string& str); // ȱʡurlת�庯��
    static string decodeUrl(const string& str); // ȱʡurl��ת�庯��
    
public:

    /**
     * ���캯��
     */
    CTrpcUrlParamMap(const string& str="");

    /**
     * ����URL������
     */
    void parseUrl(const string& strUrl);
    
    /**
     * ƴװURL
     */
    void packUrl(string& strUrl);

    /**
     * �������Ƿ�Ϊ��
     */
    bool isEmpty(const std::string& key);

protected:
    
    /**
     * ����һ��Name-value��
     */
    void parseNV(const string& strUrl, string::size_type beg, string::size_type end);
};

#endif

