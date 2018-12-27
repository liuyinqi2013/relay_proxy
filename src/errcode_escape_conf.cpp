
#include <stdio.h>
#include <stdlib.h>
#include "log.h"
#include "trpc_sys_exception.h"
#include "trpc_comm_func.h"
#include "errcode_escape_conf.h"

char *trimString(char *s, const char *cset);

ErrorCodeEscapeConf::ErrorCodeEscapeConf()
{
}

ErrorCodeEscapeConf::~ErrorCodeEscapeConf()
{
}

void
ErrorCodeEscapeConf::clean()
{
    CAutoWLock  l(_mutex);

    errcode_escape_map_.clear();
}

void 
ErrorCodeEscapeConf::read_conf(const char *config_file)
{
    CAutoWLock  l(_mutex);
    
    FILE *fp = fopen(config_file, "r");

    if (fp == NULL)
    {
        THROW(CConfigException, ERR_CONF, "error code escape conf error, invalid config file:%s", config_file);
    }

    static char filter_chars[] = { 10, 13, 32 };

    char buffer[512];
    std::vector<std::string> str_vec;
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        trimString(buffer, filter_chars);

        if (strlen(buffer) <= 1) continue;
        if (buffer[0] == '#')    continue;
   
         split(buffer, ":", 1, str_vec);
         errcode_escape_map_[atoi(str_vec[0].c_str())] = str_vec[1];

         /*
         if (str_vec.size() == 2) 
         {
             errcode_escape_map_[atoi(str_vec[0].c_str())] = str_vec[1];
         }
         else
         {
             trpc_debug_log("invalid config line:%s", buffer);
         }
         */
    }

    fclose(fp);

    //for test
    for(std::map<int, std::string>::const_iterator i = errcode_escape_map_.begin();
        i != errcode_escape_map_.end();
        ++i)
    {
        trpc_debug_log("error_code[%d], escaped text[%s]", (*i).first, (*i).second.c_str());
    }

    /*

    if (isEscaped(1003))
    {
        trpc_debug_log("1003 ESCAPED");
    }
    else
    {
        trpc_debug_log("1003 NOT ESCAPED");
    }

    if (isEscaped(1002))
    {
        trpc_debug_log("1002 escaped text:%s", getEscapedText(1002).c_str());
    }

    if (isEscaped(111111111))
    {
        trpc_debug_log("111111111 escaped text:%s", getEscapedText(111111111).c_str());
    }
    */

    return;
}

bool
ErrorCodeEscapeConf::isOriginalCode(int err_code)
{
    CAutoRLock l(_mutex);
    if (errcode_escape_map_.find(err_code) == errcode_escape_map_.end())
    {
        return false;
    }

    return   errcode_escape_map_[err_code] == "NO_ESCAPE" ;
}

bool 
ErrorCodeEscapeConf::isEscapedCode(int err_code) //const
{
    CAutoRLock l(_mutex);
    if (errcode_escape_map_.find(err_code) == errcode_escape_map_.end())
    {
        return false;
    }

    return true;
}

std::string 
ErrorCodeEscapeConf::getEscapedText(int err_code) //const
{
    CAutoRLock l(_mutex);
    return errcode_escape_map_[err_code];
}

char *trimString(char *s, const char *cset)
{
    char *start, *end, *sp, *ep;
    size_t len;

    sp = start = s;
    ep = end = s+strlen(s)-1;
    while(sp <= end && strchr(cset, *sp)) sp++;
    while(ep > start && strchr(cset, *ep)) ep--;
    len = (sp > ep) ? 0 : ((ep-sp)+1);

    char *buffer = new char[len+1];
    strncpy(buffer, sp, len);
    buffer[len] = 0;

    strcpy(s, buffer);
    delete [] buffer;

    return s;
}
