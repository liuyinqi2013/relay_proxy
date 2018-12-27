#ifndef __SERVICE_REQUEST_CONTROL_H__
#define __SERVICE_REQUEST_CONTROL_H__

#include <map>
#include "thread_mutex.h"

class ServiceControlRule;

class ServiceRequestControl
{
public:
    ServiceRequestControl();
    ~ServiceRequestControl();

    void init(const char *service_control_rule_file);
    void init(const std::vector<std::string>& rules);

    void re_init(const char *service_control_rule_file);
    void re_init(const std::vector<std::string>& rules);

    static ServiceControlRule * create(int request_type, const std::string& rule);

    void check_rule(const std::string& master_key, int error, const char *service_name, const char *request_text);

protected:
    void init_fix_i(const char *service_control_rule_file);
    void init_fix_i(const std::vector<std::string>& rules);

    void clean();

    void create_fix_rule(const std::string& one_rule);

private:
    std::map<std::string, ServiceControlRule *> service_control_rule_pool_;

    CRwLock          mutex_;
    bool             is_init_;
};

#endif
