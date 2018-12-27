#include    "trpc_sys_exception.h"

#include    "trpc_config.h"

CConfig::CConfig()
    :
    _rows(0)
{
}

CConfig::~CConfig()
{
    clear();
}

void CConfig::clear()
{
    // ?��??
    CAutoWLock l(_mutex);
    
    // �̡¨�?����������???����o����y
    _clear();
    
    // ??������y?Y
    _rows = 0;
}

void CConfig::read_conf(const char * conf_file,
        int (*fnGetRecord)(const char * str, void * arg),
        void * arg)
{
    // ?����????????��D��??
    CAutoWLock l(_mutex);
        
    FILE *fp = fopen(conf_file, "r");
    
    if (fp == NULL) 
    {
        THROW(CConfigException, ERR_CONF, "fopen %s: %s", conf_file, strerror(errno));
    }

    char buf[MAX_CONF_LENGTH_SIZE] = {0};

    for (;;) 
    {
        if (fgets(buf, sizeof(buf), fp) == NULL) 
        {
            if (feof(fp)) 
            {
                // ?����????t?����?
                break;
            }
            else 
            {
                fclose(fp);
                
                THROW(CConfigException, ERR_CONF, "fgets %s: %s",
                        conf_file, strerror(errno));
            }
        }

        /* get ride of '\n' */
        buf[MAX_CONF_LENGTH_SIZE - 1] = '\0';
        strstrip(buf);
        if (buf[0] == '#' || buf[0] == '\0')
            continue;

        // int iRetVal = get_line(buf);
        int iRetVal = fnGetRecord(buf, arg == NULL ? this:arg);
        if (iRetVal == 0) 
        {
            ++_rows;
        }
        else if (iRetVal < 0) 
        {
            fclose(fp);
            
            THROW(CConfigException, ERR_CONF, "read record error");
        }
        else
        {
            // > 0 ��?1y,2?��?��|����
        }
    }

    fclose(fp);
}

int CConfig::get_line_entrance(const char * str, void * arg)
{
    CConfig * me = (CConfig *)arg;

    try
    {
        me->get_line(str);
    }
    catch(CTrpcException& ce)
    {
        // ?????����?�䨪?��?������3���?��t??2?��?3?
        // admin ?a��?��D?��?��?����???3?��??��?��3???
        if (g_pLog != NULL)
        {
            trpc_error_log("read conf error: %s, line[%s]\n", ce.error_info(), str);
        }
        else
        {
            fprintf(stderr, "read conf error: %s, line[%s]\n", ce.error_info(), str);
        }
        
        return 1;
    }
    
    return 0;
}

void CConfig::_clear()
{
}

