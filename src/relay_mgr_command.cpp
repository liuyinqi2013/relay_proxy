#include "relay_server.h"
#include "relay_mgr_command.h"
#include "trpc_socket_opt.h"
#include "trpc_url_param_map.h"

struct TimedCommand relay_mgr_command_table[] =
{
    {NULL, FIX_FREQ_CONTROL_CMD, ServiceFixFrequenceControlCommand::create, 30, 0}
};

void create_relay_mgr_command(std::list<struct TimedCommand>& command_list, CRelayServer *relay)
{
    int numcommands = sizeof(relay_mgr_command_table) / sizeof(struct TimedCommand);

    for(int i = 0; i < numcommands; ++i)
    {
        struct TimedCommand timed_command;
        timed_command.command_ = relay_mgr_command_table[i].create_func_(relay_mgr_command_table[i].cmd_type_, relay);
        timed_command.cmd_type_ = relay_mgr_command_table[i].cmd_type_;
        timed_command.time_out_ = relay_mgr_command_table[i].time_out_;
        timed_command.time_stamp_ = time(0);

        command_list.push_back(timed_command);
    }
}

int latest_expire_command_time(const std::list<struct TimedCommand>& command_list)
{
    int t = time(0);
    for(std::list<struct TimedCommand>::const_iterator iter = command_list.begin();
        iter != command_list.end();
        ++iter)
    {
        const struct TimedCommand & command = *iter;
        
        if (t > command.time_out_)
        {
            t = command.time_out_;
        }
    }

    return t;
}

RelayMgrCommand::RelayMgrCommand(int cmd_type, CRelayServer *relay)
:cmd_type_(cmd_type), relay_(relay)
{
}

RelayMgrCommand::RelayMgrCommand()
{
}

RelayMgrCommand::~RelayMgrCommand()
{
}

/*
void
RelayMgrCommand::init(int cmd_type, CRelayServer *relay)
{
    cmd_type_ = cmd_type;
    relay_ = relay;
}
*/

ServiceFixFrequenceControlCommand::ServiceFixFrequenceControlCommand(int cmd_type, 
                                                                     CRelayServer *relay)
:RelayMgrCommand(cmd_type, relay)
{
}

ServiceFixFrequenceControlCommand::ServiceFixFrequenceControlCommand()
{
}

ServiceFixFrequenceControlCommand::~ServiceFixFrequenceControlCommand()
{
}

RelayMgrCommand *
ServiceFixFrequenceControlCommand::create(int cmd_type, CRelayServer *relay)
{
    return new ServiceFixFrequenceControlCommand(cmd_type, relay);
}

/*
void
ServiceFixFrequenceControlCommand::init(int cmd_type, CRelayServer *relay)
{
    RelayMgrCommand::init(cmd_type, relay);

    memset(buf_, 0x00, sizeof(buf_));
}
*/

void
ServiceFixFrequenceControlCommand::process()
{
    return; // add by guojt 20140722 for testing   

    std::string mgr_ip = relay_->_sys_conf.relay_mgr_ip();
    int         mgr_port = relay_->_sys_conf.relay_mgr_port();

    int  fd = 0;

    try
    {
        fd = CSocketOpt::tcp_sock();
        CSocketOpt::set_nonblock(fd, true);
        CSocketOpt::nonblock_connect(fd, mgr_ip.c_str(), mgr_port);

        std::ostringstream oss;
        oss << "command=" << cmd_type_ << "&relay_proxy_ip=" << relay_->_sys_conf.server_ip()
            << "&random_code=" << get_append_code(relay_->_sys_conf.server_ip());
        std::string cmd(oss.str());
        int len = cmd.length();

        CSocketOpt::write_relay_packet(fd, cmd.c_str(), len, 5);

        memset(buf_, 0x00, sizeof(buf_));
        CSocketOpt::read_relay_packet(fd, buf_, sizeof(buf_), 5);

        CTrpcUrlParamMap param(buf_);
        if (param.count("result") == 0 || param["result"] != "0")
        {
            trpc_debug_log("fetch rules error, %s", buf_);
            THROW(CSocketException, ERR_IPC_ERROR, "get rules error, %s", buf_);
        }

        if (param.count("rules") == 0)
        {
            trpc_debug_log("getch rules error, %s", buf_);
            THROW(CSocketException, ERR_IPC_ERROR, "get rules error, %s", buf_);
        }

        std::string rules = param["rules"];
        rules = param.decodeUrl(rules);

        if (rules != last_rules_)
        {
            std::vector<std::string> rule_vec;
            split(rules.c_str(), "&", 1024, rule_vec);

            relay_->_service_request_control.re_init(rule_vec);

            last_rules_ = rules;
        }
        
    }
    catch(CSocketException & ex)
    {
        trpc_error_log("catch CSocketException, %s", ex.what());
    }
    catch(...)
    {
    }

    close(fd);
}
