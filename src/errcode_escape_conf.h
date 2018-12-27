
#ifndef __ERROR_CODE_ESCAPE_CONF_H__
#define __ERROR_CODE_ESCAPE_CONF_H__

#include <string>
#include <map>

#include "thread_mutex.h"

class ErrorCodeEscapeConf
{
public:
    ErrorCodeEscapeConf();
    ~ErrorCodeEscapeConf();

    void clean();
    void read_conf(const char *config_file);

    bool isEscapedCode(int err_code); //const;
    bool isOriginalCode(int err_code);
    
    std::string getEscapedText(int err_code); //const;
    
    
private:
    CRwLock _mutex;
    std::map<int, std::string>  errcode_escape_map_;
};

extern ErrorCodeEscapeConf g_errcode_escape_conf;

#endif
