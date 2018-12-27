
#ifndef __SERVICE_AUTHORIZE_ACCESS_H__
#define __SERVICE_AUTHORIZE_ACCESS_H__

#include <map>
#include <vector>
#include <string>

#include "thread_mutex.h"

class ServiceAuthorizeAccess
{
public:
    ServiceAuthorizeAccess();
    ~ServiceAuthorizeAccess();

    void init(const char * config_file);

    void re_init(const char *config_file);

    bool authorized(int request_type, const char *access_ip);

private:
    void init_i(const char * config_file);

private:
    std::map<int, std::vector<std::string> >   service_authorized_set_;

    CRwLock        mutex_;
    bool           is_init_;
};

#endif
