#include "trpc_url_param_map.h"
#include <algorithm>
#include <stdio.h>

/**
 * ȱʡ�ָ���ת���ַ�
 */
const char DFL_SEG_DEL = '&';   
const char DFL_ELE_DEL = '=';

/**
* ������ڵ�һλ����Ϣ
*/
const char* __MUST_IN_FIRST_FIELD = "result";

inline char hextochar(char c1, char c2)
{
    char tmp[] = "0x00";
    int val;

    tmp[2]=c1;
    tmp[3]=c2;
    sscanf(tmp, "%x", &val);

    return static_cast<char>(val);
}

/**
 * ���徲̬��Ա����
 */
char CTrpcUrlParamMap::__SEG_DEL = DFL_SEG_DEL ; // �ηָ���
char CTrpcUrlParamMap::__ELE_DEL = DFL_ELE_DEL; // ����-ֵ�ָ���
PFUN_ESCAPE CTrpcUrlParamMap::__PFUN_ESCAPE = CTrpcUrlParamMap::encodeUrl; // ת�庯��ָ�� 
PFUN_UNESCAPE CTrpcUrlParamMap::__PFUN_UNESCAPE = CTrpcUrlParamMap::decodeUrl; // ת�庯��ָ�� 

/**
 * ȱʡurlת�庯��
 *@input: str ��ת�崮
 *@return: ת���Ĵ�
 */
string CTrpcUrlParamMap::encodeUrl(const string& str)
{
    static const char* chTable[] = {
                    "%00","%01","%02","%03","%04","%05","%06","%07","%08","%09",
                    "%0a","%0b","%0c","%0d","%0e","%0f","%10","%11","%12","%13",
                    "%14","%15","%16","%17","%18","%19","%1a","%1b","%1c","%1d",
                    "%1e","%1f"," ","%21","%22","%23","%24","%25","%26","%27",
                    "(",")","*","%2b",",","-",".","%2f","0","1",
                    "2","3","4","5","6","7","8","9",":","%3b",
                    "%3c","%3d","%3e","%3f","%40","A","B","C","D","E",
                    "F","G","H","I","J","K","L","M","N","O",
                    "P","Q","R","S","T","U","V","W","X","Y",
                    "Z","[","\\","]","%5e","_","%60","a","b","c",
                    "d","e","f","g","h","i","j","k","l","m",
                    "n","o","p","q","r","s","t","u","v","w",
                    "x","y","z","%7b","|","%7d","~","%7f","%80","%81",
                    "%82","%83","%84","%85","%86","%87","%88","%89","%8a","%8b",
                    "%8c","%8d","%8e","%8f","%90","%91","%92","%93","%94","%95",
                    "%96","%97","%98","%99","%9a","%9b","%9c","%9d","%9e","%9f",
                    "%a0","%a1","%a2","%a3","%a4","%a5","%a6","%a7","%a8","%a9",
                    "%aa","%ab","%ac","%ad","%ae","%af","%b0","%b1","%b2","%b3",
                    "%b4","%b5","%b6","%b7","%b8","%b9","%ba","%bb","%bc","%bd",
                    "%be","%bf","%c0","%c1","%c2","%c3","%c4","%c5","%c6","%c7",
                    "%c8","%c9","%ca","%cb","%cc","%cd","%ce","%cf","%d0","%d1",
                    "%d2","%d3","%d4","%d5","%d6","%d7","%d8","%d9","%da","%db",
                    "%dc","%dd","%de","%df","%e0","%e1","%e2","%e3","%e4","%e5",
                    "%e6","%e7","%e8","%e9","%ea","%eb","%ec","%ed","%ee","%ef",
                    "%f0","%f1","%f2","%f3","%f4","%f5","%f6","%f7","%f8","%f9",
                    "%fa","%fb","%fc","%fd","%fe","%ff"};

    string result;
    for(string::const_iterator it= str.begin(); it!=str.end(); ++it)
    {
        result += chTable[static_cast<unsigned char>(*it)];
    }

    return result;
}

/**
 * ȱʡurl��ת�庯��
 *@input: str ת�崮
 *@return: ת��ǰ�Ĵ�
 */
string CTrpcUrlParamMap::decodeUrl(const string& str)
{
    string result;
  
    for(string::const_iterator it= str.begin(); it!=str.end(); ++it)
    {
        if (*it ==  '%' && std::distance(it, str.end()) >= 3)
        {
            result.append(1, hextochar(*(it+1), *(it+2)));
            std::advance(it, 2);
        }
        else 
        {
            result.append(1, *it);
        }
    }
    
    return result;
}

/**
 * ���캯��
 */
CTrpcUrlParamMap::CTrpcUrlParamMap(const string& str)
{
    parseUrl(str);
}

/**
 * ����URL������
 */
void CTrpcUrlParamMap::parseUrl(const string& strUrl)
{
    map<string, string>::clear();

    string::size_type q = 0;
    string::size_type p = 0;
    
    while(q != strUrl.npos)
    {
        p = strUrl.find(__SEG_DEL, q);
        
        parseNV(strUrl, q, p);

        q = (p == strUrl.npos) ? strUrl.npos : ++p;
    }
}
    
/**
 * �������map
 */
void CTrpcUrlParamMap::packUrl(string& result)
{
    result.clear();

    iterator tmp = find(__MUST_IN_FIRST_FIELD);

    if (tmp != end())
    {
        result += tmp->first;
        result += __ELE_DEL;
        result += (*__PFUN_ESCAPE)(tmp->second);
        result += __SEG_DEL;
    }

    for(iterator it = begin(); it != end(); ++it)
    {
        if (it->first == __MUST_IN_FIRST_FIELD)
        {
            continue;
        }
    
        result += it->first;
        result += __ELE_DEL;
        result += (*__PFUN_ESCAPE)(it->second);
        result += __SEG_DEL;
    }
}

/**
 * �������Ƿ�Ϊ��
 */
bool CTrpcUrlParamMap::isEmpty(const std::string& key)
{
    iterator tmp = find(key);

    if (tmp == end())
    {
        return true;
    }

    if (tmp->second.length() == 0)
    {
        return true;
    }

    return false;
}

/**
 * ����һ��Name-value��
 */
void CTrpcUrlParamMap::parseNV(const string& strUrl, string::size_type beg, string::size_type end)
{
    string::size_type pos = strUrl.find(__ELE_DEL, beg);

    if(pos != string::npos && pos <= end)
    {
        map<string, string>::value_type item(strUrl.substr(beg, pos-beg), (*__PFUN_UNESCAPE)(strUrl.substr(pos+1, end - pos - 1)));
        
        map<string, string>::insert(item);
    }
}

