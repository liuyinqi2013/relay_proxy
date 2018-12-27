#include    <string>
#include    "log_mutex.h"
#include    "relay_log.h"

extern class CLog * g_pLog;
extern class CLog * g_pAccLog;
extern class CLog * g_pStatsLog;

void init_log(const char * log_path, int log_size, int log_num)
{
    // 沿用以前的日志名字
    std::string app_log_name = std::string(log_path) + "/APP_20";
    std::string acc_log_name = std::string(log_path) + "/ACCESS_20";
    std::string stats_log_name = std::string(log_path) + "/service_stats";
    
    g_pLog = new CLog(app_log_name.c_str(), log_size, log_num);

    g_pAccLog = new CLog(acc_log_name.c_str(), log_size, log_num);

    g_pStatsLog = new CLog(stats_log_name.c_str(), log_size, log_num);
}

void fini_log()
{
    delete g_pStatsLog;
    g_pStatsLog = NULL;

    delete g_pAccLog;
    g_pAccLog = NULL;

    delete g_pLog;
    g_pLog = NULL;
}
