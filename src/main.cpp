#include    <stdio.h>
#include    <stdlib.h>
#include    <signal.h>

#include    "relay_server.h"

// 全局变量 -- reload 使用
CRelayServer relay_server;

void sig_reload(int sig)
{
    try
    {
        relay_server.reload_conf();
    }
    catch(CTrpcException& ce)
    {
        trpc_error_log("reload_conf CTrpcException: %s\n", ce.error_info());
    }
    catch(std::exception& ce)
    {
        trpc_error_log("reload_conf std::exception: %s\n", ce.what());
    }
    catch(...)
    {
        trpc_error_log("reload_conf unknown error\n");
    }   
}

void init_sig_conf()
{
    struct sigaction act;
    act.sa_handler = sig_reload;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_RESTART;
    if(sigaction(SIGHUP, &act, NULL) < 0) 
    {
        fprintf(stderr, "set sig conf error: %s", strerror(errno));

        exit(-1);
    }
}

int write_pid(void)
{
    static const char * pid_file = "/tmp/relay_svr.pid";

    FILE *fp = fopen(pid_file, "w+");
    if (fp == NULL)
    {
        fprintf(stderr, "open pid file [%s] error : %s\n", pid_file, strerror(errno));  
        return -1;
    }

    pid_t pid = getpid();

    // 写入文件
    fprintf(fp, "%u", pid);

    // 关闭文件,只有启动时候会写一次
    if (fclose(fp) != 0)
    {
        fprintf(stderr, "close pid file error : %s\n", strerror(errno));  
        return -1;
    }

    return 0;
}

int main()
{
    init_daemon();
    write_pid();
    init_sig_conf();

    try
    {
        relay_server.init();

        relay_server.process();

        // 一般不会到这里
        relay_server.fini();
    }
    catch(CTrpcException& ce)
    {
        fprintf(stderr, "CTrpcException: %s\n", ce.error_info());

        relay_server.fini();
    }
    catch(std::exception& ce)
    {
        fprintf(stderr, "std::exception: %s\n", ce.what());

        relay_server.fini();
    }
    catch(...)
    {
        fprintf(stderr, "unknown error\n");

        relay_server.fini();
    }

    exit(0);
}
