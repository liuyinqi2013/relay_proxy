#include    "frequency_control.h"
#include    "trpc_sys_exception.h"
#include <stdlib.h>

CFrequencyControl::CFrequencyControl()
{
}

CFrequencyControl::~CFrequencyControl()
{
    clear();
}
    
void CFrequencyControl::_clear()
{
    _map.clear();
}

int CFrequencyControl::get_line(const char * str)
{
    // �׳����쳣�ڻ������д���,�������Ƴ�,ֻ�ᱨ��,������Ӱ��,���ﲻ����
    return _get_line(str);
}

// ��ȡ������Ϣ
int CFrequencyControl::_get_line(const char * str)
{
        // �����ļ�����ð�ŷָ�
    static const char *   sep = ":";

    char *          tmp = NULL; // ������strtok_r�Ĳ���
    char *          tok = NULL;
    
    char t[MAX_CONF_LENGTH_SIZE] = {0};

    int     request_type;
    char    ip[MAX_IP_LEN] = {0};

    ST_FrequencyControlData frequency_data;
    memset(&frequency_data, 0x00, sizeof(frequency_data));
        
    // ����һ����ʱ�ַ���,���޸�str
    strncpy(t, str, sizeof(t) - 1);

    // request_type
    tok = strtok_r(t, sep, &tmp);
    if (tok == NULL)
    {
        THROW(CConfigException, ERR_CONF, "FreqLimit_conf: request_type not find[%s]", str);
    }
    else
    {
        request_type = atoi(strstrip(tok));
    }

    // ip
    tok = strtok_r(NULL, sep, &tmp);
    if (tok == NULL)
    {
        THROW(CConfigException, ERR_CONF, "FreqLimit_conf: ip not find[%s]", str);
    }
    else
    {
        strncpy(ip, strstrip(tok), sizeof(ip) - 1);
    }

    // ��ȡ���õ�ֵ
    // limit_time
    tok = strtok_r(NULL, sep, &tmp);
    if (tok == NULL)
    {
        THROW(CConfigException, ERR_CONF, "FreqLimit_conf: limit_time not find[%s]", str);
    }
    else
    {
        frequency_data.limit_time = atoi(strstrip(tok));
    }

    // limit_count
    tok = strtok_r(NULL, sep, &tmp);
    if (tok == NULL)
    {
        THROW(CConfigException, ERR_CONF, "FreqLimit_conf: limit_count not find[%s]", str);
    }
    else
    {
        frequency_data.limit_count = atoi(strstrip(tok));
    }  

    frequency_data.begin_time = time(NULL);
    frequency_data.call_times = 0;

    // ����map
    CFrequencyControlKey key(request_type, ip);
    _map[key] = frequency_data;

    trpc_debug_log("get frelimit_conf, ip: %s, request_type: %d, limit time: %d, limit count: %d",
                    ip, request_type, frequency_data.limit_time, frequency_data.limit_count);
    
    return 0;
}

void CFrequencyControl::check(const char * ip, const int request_type)
{
    // �Ӷ�����ֱ���ۼӲ�����г�ͻҲ��Ӱ�죬ͳ�����ݲ���Ҫ�ܾ�ȷ
    CAutoRLock l(_mutex);

    _check(ip, request_type);
}

void CFrequencyControl::_check(const char * ip, const int request_type)
{
    // ���
    CFrequencyControlKey t_key(request_type, ip);

    trpc_debug_log("check, ip: %s, request_type: %d", ip, request_type);

    // ����Ƿ��������
    if (_map.find(t_key) == _map.end())
    {
        // ������request_type + ip�����ƣ�ֱ�ӷ��ؼ���
        //trpc_debug_log("check, ip: %s, request_type: %d, not in map", ip, request_type);

        return;
    }

    // ��ȡ�ڵ���Ϣ
    ST_FrequencyControlData& data = _map[t_key];

    time_t now = time(NULL);

    if ((data.begin_time + data.limit_time) < now)
    {
        // �Ѿ��������ƣ����������Ϣ
        data.begin_time = now;
        data.call_times = 0;
    }
    else
    {
        // �ۼ�����
        if (++data.call_times > data.limit_count)
        {
            // �������ƣ����������
            THROW(CServerException, ExceedFreqLimit, "ip: %s, request_type: %d, begin times: %d, now: %d frequency limit is : %d second %d times",    
                        ip, request_type, (int)data.begin_time, (int)now, data.limit_time, data.limit_count);
        }
    }
    
    trpc_debug_log("check frelimit_conf, ip: %s, request_type: %d, begin_time: %d, limit time: %d, limit count: %d, call_times: %d, now: %d", ip, request_type, (int)data.begin_time, data.limit_time, data.limit_count, data.call_times, (int)now);

}

