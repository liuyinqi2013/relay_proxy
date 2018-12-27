#include    <ctype.h>
#include    <unistd.h>
#include    <signal.h>
#include    <sys/types.h>
#include    <sys/stat.h>
#include    <libgen.h>

#include    "trpc_comm_func.h"

/* tlinux 64 部分版本strcpy使用了sse3优化以后，dst和src地址相差15以下，会出问题，所以采用这个函数 */
char *mstrcpy(char *strDestination,const char *strSource)
{
    char *strD=strDestination;
    while ((*strDestination++=*strSource++)!='\0');
    return strD;
}

/* blame Elliot for these next five routines */
char * strchug(char *string)
{
    char *start;

    for (start = string; *start && isspace(*start); start++);

    mstrcpy(string, start);

    return string;
}


char * strchomp(char *string)
{
    char *s;

    if (!*string)
        return string;

    for (s = string + strlen(string) - 1; s >= string && isspace(*s); s--)
        *s = '\0';

    return string;
}

void split(const char * string,
            const char * delimiter,
            int max_tokens,
            std::vector<std::string>& data)
{
    data.clear();

    // 去掉前面的空格
    while (isspace(*string)) {
        ++string;
    }

    const char *s = strstr(string, delimiter);

    if (s) {
        int del_len = strlen(delimiter);

        do {
            int len = s - string;
            int rel_len = len;
            // 去掉后面的空格

            for (const char *p = s - 1; p > string && isspace(*p); --p) {
                --rel_len;
            }
            data.push_back(std::string(string, rel_len));

            string = s + del_len;

            // 去掉前面的空格
            while (isspace(*string)) {
                ++string;
            }
            s = strstr(string, delimiter);
        }
        while (--max_tokens && s && *s);
    }

    if (*string) {
        // 去掉后面的空格
        int rel_len = strlen(string);
        for (const char *p = string + rel_len - 1;
        p > string && isspace(*p);
        --p) {
            --rel_len;
        }
        data.push_back(std::string(string, rel_len));
    }

    return;
}

void set_mask(int& base, int mask)
{
    base |= mask;
}

void unset_mask(int& base, int mask)
{
    base &= (!mask);
}

bool check_mask(int base, int mask)
{
    return base & mask;
}

void clean_mask(int& base)
{
    base &= 0x0;
}

void init_daemon()
{
    daemon(1, 0);

    signal(SIGINT, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    
    ignore_pipe();
}

void ignore_pipe()
{
    struct sigaction sig;

    sig.sa_handler = SIG_IGN;
    sig.sa_flags = 0;
    sigemptyset(&sig.sa_mask);
    sigaction(SIGPIPE, &sig, NULL);
}

int is_number(const char * buf)
{
    const char * p = buf;

    if (p == NULL)
    {
        return -1;
    }

    while (*p != '\0')
    {
        if (! isdigit(*p))
        {
            return -1;
        }
        p++;
    }

    return 0;
}

// 加入对server名字的判断
int check_process(pid_t pid, const char * process_name)
{
    char patch[256];
    snprintf(patch, sizeof(patch), "/proc/%d", pid);

    struct stat stStat;
    if (stat(patch, &stStat) < 0) {
        return -1;
    }

    // 获取服务名字检查
    char cmd_path[256];
    snprintf(cmd_path, sizeof(cmd_path), "%s/cmdline", patch);

    FILE * fp = fopen(cmd_path, "r");
    if (fp == NULL)
    {
         return -1;
    }

    char cmd[256] = {0};
    size_t  len = sizeof(cmd);
    if(fread(cmd, sizeof(char), len, fp) <= 0)
    {
        fclose(fp);
        
        return -1;
    }

    // 检查服务名字
    if (strcmp(basename(cmd), process_name) != 0)
    {
        fclose(fp);
        
        return -1;
    }
    
    fclose(fp);
    
    return 0;
}

char * time2longstr(time_t t, char *strtime)
{
    struct tm tm;

    localtime_r(&t, &tm);
    strftime(strtime, 20, "%Y-%m-%d %H:%M:%S", &tm);

    return strtime;
}

// 创建目录
void create_path(const char * file)
{
    // 如果日志目录不存在，创建目录
    char log_dir[256];
    strcpy(log_dir, file);
    strcpy(log_dir, dirname(log_dir));
    struct stat dir_stat;
    if (stat(log_dir, &dir_stat) != 0) 
    {
        if (mkdir(log_dir, 0775) < 0)
        {
            fprintf(stderr, "mkdir %s: %s\n", log_dir, strerror(errno));
        }
    }
}

unsigned char get_hour()
{
    time_t tNow = time(NULL);
    struct tm tm;
    localtime_r(&tNow, &tm);

    return (unsigned char)tm.tm_hour;
}

#define MAX_DRU_HASH_NODES 1021

int get_append_code(const char* key)
{
    if (key == NULL)
    {
        return -1;
    }

    int len = strlen(key);
    if (len == 0)
    {
        return -1;
    }

    uint32_t hash = 0;
    const char * ptr = key;
    
    while (0 < len--  && *ptr)
    {
        hash = hash << 1 ^ *ptr++;
    }

    return (hash % MAX_DRU_HASH_NODES);
}
