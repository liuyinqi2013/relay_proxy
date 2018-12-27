#ifndef	    __SOCKET_OPT_H__
#define     __SOCKET_OPT_H__

/**
 *  对socket属性的设置的公共方法
 */

#include    <ctype.h>
#include    <string>

#include    <sys/types.h>
#include    <sys/socket.h>
#include    <stdint.h>

const int INVALID_SOCKET = -1;

// 封装socket的操作, 放到单独的名字空间中, 避免和其他的服务或者系统api冲突
namespace CSocketOpt
{ 
    // socket create func
    int create(int domain, int type, int protocol);
    int unix_tcp_sock();
    int tcp_sock();
    int udp_sock();

    void close(int& fd);

    // bind func
    int bind(int fd, struct sockaddr * addr, socklen_t addrlen);
    
    void unix_bind(int fd, const char * address);
    void net_bind(int fd, const std::string& ip, const int port);
    void net_bind(int fd, const char * address);

    // listen
    void listen(int fd, int backlog = 5);
    void nonblock_listen(int fd, int backlog = 5);

    // connect
    // 所有的connect默认超时时间都是1秒
    void unix_connect(int fd, const char * dst_address, int time_out);
    
    void net_connect(int fd, const char * ip, const int port, int time_out);
    void net_connect(int fd, const char * dst_address, int time_out);
    
    void asyn_connect(int fd, struct sockaddr * bind_addr, socklen_t addrlen, int time_out);
    
    void nonblock_connect(int fd, const char * ip, const int port);
    void nonblock_connect(int fd, struct sockaddr * bind_addr, socklen_t addrlen);
    void check_connect_status(int fd);

    // accept
    int accept(int fd);

    // read/write
    ssize_t read(int fd, void *buf, size_t count);
    ssize_t read_all(int fd, void * buf, size_t count, int time_out);
    ssize_t write(int fd, const void * buf, size_t count);
    ssize_t write_all(int fd, const void * buf, size_t count, int time_out);
    ssize_t send_to(int fd, const void *msg, size_t len,
                    uint16_t port, const char * ip,
                    int flags = 0);
    ssize_t send_to(int fd, const void * msg, size_t len,
                    const struct sockaddr * to, socklen_t tolen,
                    int flags = 0);
   
    
    ssize_t write_relay_packet(int fd, const void *buffer, int length, int time_out);
    ssize_t read_relay_packet(int fd, void *buffer, int length, int time_out);

    // socket opt func
    
    void get_sock_opt(int fd, int optname, void *optval, socklen_t * optlen, int level = SOL_SOCKET);
    void set_sock_opt(int fd, int optname, const void *optval, socklen_t optlen, int level = SOL_SOCKET);
    void set_nonblock(int fd, bool flag = true); 
    void set_keep_alive(int fd, int keep_alive = 1, int keep_idle = 600, int keep_interval = 5, int keep_count = 3);
    void set_reuser_addr(int fd);

    bool check_tcp_established(int fd);

    // other
    int terminal_error(int error_no);
    bool is_ok(int fd);
    bool check_tcp_ok(int fd);

    void get_peer_addr(int fd, std::string& ip, int & port);
    void get_sock_addr(int fd, std::string& ip, int & port);

    void parse_host(const char * address, std::string& ip, int& port);

    unsigned long int get_hex_ip();
    char * hexIP_to_normal(unsigned long int hex_ip, char * normal_ip);
    char * get_local_ip(char * ip);

    // 链接超时2秒足够
    static const int DEFAULT_NET_CONNECT_TIME_OUT = 2;
}

#endif

