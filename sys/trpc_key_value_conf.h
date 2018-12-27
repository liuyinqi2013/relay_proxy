#ifndef     __TRPC_KEY_VALUE_CONF_H__
#define     __TRPC_KEY_VALUE_CONF_H__


#include    <vector>

#include    "trpc_config.h"

struct key_value_info{
    char key[32];
    char value[2048];
};

class CKeyValueConf: public CConfig
{
public:
    CKeyValueConf();
    virtual ~CKeyValueConf();

    virtual void    read_conf(const char * conf_file);
   
    const char *    get_value(const char * key);
    
    virtual void    clear();

protected:
    virtual int     get_line(const char * str);
    
protected:
    static int      comp(const void * p1, const void * p2);
    static int      search_comp(const void * p1, const void * p2);
    
protected:
    std::vector<key_value_info>    _data;
};

#endif

