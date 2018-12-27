#ifndef __RELAY_MGR_COMMAND_H__
#define __RELAY_MGR_COMMAND_H__

#include <string>
#include <list>

const int FIX_FREQ_CONTROL_CMD = 1000;
const int ADAPTER_FREQ_CONTROL_CMD = 1001;

class CRelayServer;
class RelayMgrCommand;

typedef RelayMgrCommand * (*CreateCommand)(int cmd_type, CRelayServer *relay);

struct TimedCommand
{
    RelayMgrCommand * command_;
    int           cmd_type_;
    CreateCommand create_func_; 

    int           time_out_;
    time_t        time_stamp_;
};

extern struct TimedCommand relay_mgr_command_table[];

extern void create_relay_mgr_command(std::list<struct TimedCommand>& command_list, CRelayServer *relay);
extern int latest_expire_command_time(const std::list<struct TimedCommand>& command_list);

class RelayMgrCommand
{
public:
    RelayMgrCommand(int cmd_type, CRelayServer *relay);
    RelayMgrCommand();

    virtual ~RelayMgrCommand() = 0;

    virtual void process() = 0;

    //virtual void init(int cmd_type, CRelayServer *relay);

protected:
    int            cmd_type_;
    CRelayServer  *relay_;
};

class ServiceFixFrequenceControlCommand : public RelayMgrCommand
{
public:
    ServiceFixFrequenceControlCommand(int cmd_type, CRelayServer *relay);
    ServiceFixFrequenceControlCommand();

    ~ServiceFixFrequenceControlCommand();

    virtual void process();

    static RelayMgrCommand *create(int cmd_type, CRelayServer *relay);

    //virtual void init(int cmd_type, CRelayServer *relay);

private:
    char   buf_[32*1024];

    std::string last_rules_;
};


#endif
