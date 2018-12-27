#ifndef     __TRPC_COMM_FUNC_H__
#define     __TRPC_COMM_FUNC_H__

#include    <string>
#include    <vector>
#include    <sstream>
#include    <netinet/in.h>
#include    <arpa/inet.h>

#include    "trpc_sys_exception.h"

/**
 *  ¹«¹²º¯Êý
 *
 *
 */

char *strchug (char *string);
char * strchomp (char *string);
#define strstrip(string)    strchomp(strchug(string))

void split(const char * string,
            const char * delimiter,
            int max_tokens,
            std::vector<std::string>& data);

void set_mask(int& base, int mask);
void unset_mask(int& base, int mask);
bool check_mask(int base, int mask);
void clean_mask(int& base);

void init_daemon();
void ignore_pipe();

int is_number(const char * buf);
int check_process(pid_t pid, const char * process_name);

char * time2longstr(time_t t, char *strtime);

void create_path(const char * file);

unsigned char get_hour();

int get_append_code(const char* key);

template< class T >
const std::string to_string(const T& t)
{
    std::stringstream ss;

    ss << t;

    return ss.str();
}

template<typename T, typename R>
R type_cast(const T& t)
{
    std::stringstream ss;
 
    ss << t;

    R r;
    ss >> r;

    return r;
}

#endif


