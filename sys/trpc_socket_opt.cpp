
#include    <unistd.h>
#include    <stdio.h>
#include    <stdlib.h>

#include    <netdb.h>
#include    <fcntl.h>

#include    <sys/un.h>
#include    <sys/types.h>
#include    <sys/socket.h>

#include    <arpa/inet.h>
#include    <netinet/tcp.h>
#include    <netinet/in.h>

//#include    <stropts.h>
#include    <sys/ioctl.h>
#include    <linux/if.h>

#include    "trpc_socket_opt.h"
#include    "trpc_sys_exception.h"
#include    "trpc_comm_func.h"

int CSocketOpt::create(int domain, int type, int protocol)
{
    int fd = INVALID_SOCKET;
    
    if ((fd = ::socket(domain, type, protocol)) < 0)
    {
        THROW(CSocketException, ERR_SOCKET_TERMINAL, "create: %s", strerror(errno));
    }

    return fd;
}

int CSocketOpt::unix_tcp_sock()
{
    return create(AF_UNIX, SOCK_STREAM, 0);
}

int CSocketOpt::tcp_sock()
{
    return create(AF_INET, SOCK_STREAM, 0);
}

int CSocketOpt::udp_sock()
{
    return create(AF_INET, SOCK_DGRAM, 0);
}

void CSocketOpt::close(int& fd)
{
    if (fd != INVALID_SOCKET)
    {
        int t = fd;
        fd = INVALID_SOCKET;
        ::close(t);
    }
}

int CSocketOpt::bind(int fd, struct sockaddr * addr, socklen_t addrlen)
{
    return ::bind(fd, addr, addrlen);
}

void CSocketOpt::unix_bind(int fd, const char * address)
{
    /**
     *  sockaddr_un 实现为 需要判断路径超长
     *      #define UNIX_PATH_MAX   108
     *
     *       struct sockaddr_un {
     *               sa_family_t sun_family; 
     *               char sun_path[UNIX_PATH_MAX];   
     *       };
     *
     */
    
    struct sockaddr_un server_address;     

    memset(&server_address, 0x00, sizeof(server_address));

    // 检查路径是否超长
    if (strlen(address) >= sizeof(server_address.sun_path))
    {
        THROW(CSocketException, ERR_SOCKET_TERMINAL, "unix_bind address: %s too long", address);
    }

    // 先删除以前的socket，否则无法绑定
    if (unlink(address) != 0) 
    {
        if (errno != ENOENT) 
        {
            THROW(CSocketException, ERR_SOCKET_TERMINAL, "unix_bind unlink %s: %s", address, strerror(errno));
        }
    }

    server_address.sun_family = AF_UNIX;
    strcpy (server_address.sun_path, address);

    if (bind(fd, (struct sockaddr *)&server_address, (socklen_t)sizeof(server_address)) < 0)
    {
        THROW(CSocketException, ERR_SOCKET_TERMINAL, "unix_bind: %s, error: %s", address, strerror(errno));
    }
}

void CSocketOpt::net_bind(int fd, const std::string& ip, const int port)
{
    // 获取ip地址
    struct  addrinfo    *ailist;
    struct  addrinfo    hint;

    int                 err;

    // 防止报警。。。
    ailist  = NULL;
    
    memset(&hint, 0x00, sizeof(hint));
    hint.ai_flags = AI_CANONNAME|AI_PASSIVE;
    hint.ai_family = AF_INET;

    hint.ai_socktype = SOCK_STREAM;
    hint.ai_protocol = IPPROTO_TCP;

    hint.ai_addrlen = 0;
    hint.ai_canonname = NULL;
    hint.ai_addr = NULL;
    hint.ai_next = NULL;

    if ((err = getaddrinfo(ip.c_str(), to_string(port).c_str(), &hint, &ailist)) != 0)
    {
        THROW(CSocketException, ERR_SOCKET_TERMINAL, "getaddrinfo error: %s\n", gai_strerror(err));
    }
    
    set_reuser_addr(fd);

    // 只取第一个ip即可
    if (bind(fd, ailist->ai_addr, ailist->ai_addrlen) < 0)
    {
        freeaddrinfo(ailist);

        THROW(CSocketException, ERR_SOCKET_TERMINAL, "net_bind %s:%d error: %s", ip.c_str(), port, strerror(errno));
    }
    
    freeaddrinfo(ailist);
}

void CSocketOpt::net_bind(int fd, const char * address)
{
    std::string ip;
    int         port;

    parse_host(address, ip, port);

    net_bind(fd, ip, port);
}

void CSocketOpt::listen(int fd, int backlog /* = 5 */)
{        
    if (::listen(fd, backlog) < 0)
    {
        THROW(CSocketException, ERR_SOCKET_TERMINAL, "listen error: %s", strerror(errno));
    }
}

void CSocketOpt::nonblock_listen(int fd, int backlog /* = 5 */)
{
    set_nonblock(fd);

    listen(fd, backlog);//ximi add backlog at 20160822
}

void CSocketOpt::unix_connect(int fd, const char * dst_address, int time_out)
{
    // 超时的参数暂时不做处理
    if (!is_ok(fd))
    {
        THROW(CSocketException, ERR_SOCKET_TERMINAL, "unix_connect error: INVALID_SOCKET");
    }
    
    struct sockaddr_un address;
    
    // 检查路径是否超长
    if (strlen(dst_address) >= sizeof(address.sun_path))
    {
        THROW(CSocketException, ERR_SOCKET_TERMINAL, "unix_connect address too long: %s", dst_address);
    }

    int len = sizeof(address);
    
    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, dst_address);

    // 直接链接，不用异步和超时
    if (::connect(fd, (struct sockaddr *)&address, len) != 0)
    {
        // 没文件和被拒绝都认为链接失败
        if (errno == ECONNREFUSED || errno == ENOENT)
        {
            // 链接被拒绝单独错误码处理
            THROW(CSocketException, ERR_SOCKET_CONN_REFUSED, "asyn_connect error: %s", strerror(errno));
        }
        else
        {
            THROW(CSocketException, ERR_SOCKET_TERMINAL, "asyn_connect error: %s", strerror(errno));
        }
    }
    
    // CSocketOpt::asyn_connect(fd, (struct sockaddr *)&address, len, time_out);
}

void CSocketOpt::asyn_connect(int fd, struct sockaddr * bind_addr, socklen_t addrlen, int time_out)
{    
    // 保存信息
    int back_stat = fcntl(fd, F_GETFL, 0);

    if (back_stat < 0)
    {
        THROW(CSocketException, ERR_SOCKET_TERMINAL, "fcntl error: %s", strerror(errno));
    }

    // 设置为非阻塞
    set_nonblock(fd);

    // 测试，先设置为阻塞
    // set_nonblock(fd, false);
    int ret = ::connect(fd, (struct sockaddr *)bind_addr, addrlen);
    // set_nonblock(fd);

    if (ret < 0)
    {
		if (errno == EINPROGRESS || errno == EAGAIN)
		{
            // 重入
			fd_set rset, wset;
			int num;
            
            struct timeval t;
            t.tv_sec = time_out;
            t.tv_usec = 0;
                
			FD_ZERO(&rset );
			FD_SET(fd, &rset );
			FD_ZERO( &wset );
			FD_SET(fd, &wset );

			num = select(fd + 1, &rset, &wset, NULL, &t);
			if (num == 0) 
            {
				// time out
				THROW(CSocketException, ERR_SOCKET_TIME_OUT, "asyn_connect time out: %d", time_out);
			}
            else if (num < 0)
            {
                THROW(CSocketException, ERR_SOCKET_TERMINAL, "connect error: %s", strerror(errno));
            }
            
			if (!FD_ISSET(fd, &rset) && !FD_ISSET(fd, &wset))
			{
                THROW(CSocketException, ERR_SOCKET_TERMINAL, "asyn_connect error: FD_ISSET");
			}

			// 获取返回信息    
			// 保存connect返回的socket信息
            int socket_ret = 0;
			socklen_t socket_ret_length = sizeof(socket_ret);
            get_sock_opt(fd, SO_ERROR, (char*)&socket_ret, &socket_ret_length);

            if (socket_ret != 0)
            {
                THROW(CSocketException, ERR_SOCKET_TERMINAL, "connect error socket_ret %d, %s", 
                        socket_ret, strerror(errno));
            }

		}
        else
        {
            // 没文件和被拒绝都认为链接失败
            if (errno == ECONNREFUSED || errno == ENOENT)
            {
                // 链接被拒绝单独错误码处理
                THROW(CSocketException, ERR_SOCKET_CONN_REFUSED, "asyn_connect error: %s", strerror(errno));
            }
            else
            {
                THROW(CSocketException, ERR_SOCKET_TERMINAL, "asyn_connect error: %s", strerror(errno));
            }
        }
    }

    // 恢复以前的状态
    fcntl(fd, F_SETFL, back_stat);
}

void CSocketOpt::net_connect(int fd, const char * ip, const int port, int time_out)
{
    // 获取ip地址
    struct  addrinfo    *ailist;
    struct  addrinfo    hint;
    
    int                 err;

    // 防止报警
    ailist  = NULL;

    memset(&hint, 0x00, sizeof(hint));
    hint.ai_flags = AI_CANONNAME;
    hint.ai_family = AF_INET;

    hint.ai_socktype = SOCK_STREAM;
    hint.ai_protocol = IPPROTO_TCP;

    hint.ai_addrlen = 0;
    hint.ai_canonname = NULL;
    hint.ai_addr = NULL;
    hint.ai_next = NULL;

    if ((err = getaddrinfo(ip, to_string(port).c_str(), &hint, &ailist)) != 0)
    {
        THROW(CSocketException, ERR_SOCKET_TERMINAL, "net_connect getaddrinfo error: %s\n", gai_strerror(err));
    }

    try
    {
        asyn_connect(fd, ailist->ai_addr, ailist->ai_addrlen, time_out);
    }
    catch(...)
    {
        freeaddrinfo(ailist);

        throw;
    }
    
    freeaddrinfo(ailist);
}

void CSocketOpt::net_connect(int fd, const char * dst_address, int time_out)
{
    std::string ip;
    int         port;

    parse_host(dst_address, ip, port);

    net_connect(fd, ip.c_str(), port, time_out);
}

void CSocketOpt::nonblock_connect(int fd, const char * ip, const int port)
{
    // 获取ip地址
    struct  addrinfo    *ailist;
    struct  addrinfo    hint;
    
    int                 err;

    // 防止报警
    ailist  = NULL;

    memset(&hint, 0x00, sizeof(hint));
    hint.ai_flags = AI_CANONNAME;
    hint.ai_family = AF_INET;

    hint.ai_socktype = SOCK_STREAM;
    hint.ai_protocol = IPPROTO_TCP;

    hint.ai_addrlen = 0;
    hint.ai_canonname = NULL;
    hint.ai_addr = NULL;
    hint.ai_next = NULL;

    if ((err = getaddrinfo(ip, to_string(port).c_str(), &hint, &ailist)) != 0)
    {
        THROW(CSocketException, ERR_SOCKET_TERMINAL, "net_connect getaddrinfo error: %s\n", gai_strerror(err));
    }

    try
    {
        nonblock_connect(fd, ailist->ai_addr, ailist->ai_addrlen);
    }
    catch(...)
    {
        freeaddrinfo(ailist);

        throw;
    }
    
    freeaddrinfo(ailist);
}

void CSocketOpt::nonblock_connect(int fd, struct sockaddr * bind_addr, socklen_t addrlen)
{
    // 直接调用connect，然后外围自己做select等结果
    int ret = ::connect(fd, (struct sockaddr *)bind_addr, addrlen);

    if (ret < 0)
    {
        if (terminal_error(errno))
        {
            THROW(CSocketException, ERR_SOCKET_TERMINAL, "connect error %d: %s", errno, strerror(errno));
        }
    }
}

void CSocketOpt::check_connect_status(int fd)
{
    // 检查nonblock_connect结果 
    int socket_ret = 0;
	socklen_t socket_ret_length = sizeof(socket_ret);
    get_sock_opt(fd, SO_ERROR, (char*)&socket_ret, &socket_ret_length);

    if (socket_ret != 0)
    {
        THROW(CSocketException, ERR_SOCKET_TERMINAL, "connect error socket_ret %d, %s", 
                socket_ret, strerror(errno));
    }
}

int CSocketOpt::accept(int fd)
{
    int client_fd = INVALID_SOCKET;
    
    if ((client_fd = ::accept(fd, NULL, NULL)) < 0)
    {
        if (terminal_error(errno))
        {
            THROW(CSocketException, ERR_SOCKET_TERMINAL, "accept error %d: %s", errno, strerror(errno));
        }
        else
        {
            // 出错返回-1
            return -1;
        }
    }

    return client_fd;
}

ssize_t CSocketOpt::read(int fd, void *buf, size_t count)
{
    if (!is_ok(fd))
    {
        THROW(CSocketException, ERR_IPC_ERROR, "read error: invalid socket");
    }
    
    ssize_t n;

    n = ::read(fd, buf, count);

    if (n == 0)
    {
        // 返回为0，recv_close
        THROW(CSocketException, ERR_SOCKET_RECV_CLOSE, "recv close fd: %d", fd);
    }
    else if (n < 0)
    {
        if (terminal_error(errno))
        {
            THROW(CSocketException, ERR_IPC_ERROR, "read: %s", strerror(errno));
        }
    }

    return n;
}

ssize_t CSocketOpt::send_to(int fd, const void *msg, size_t len,
                uint16_t port, const char * ip /*= NULL*/ ,
                int flags /* = 0 */ )
{
    ssize_t bytes = -1;
    
    // 获取ip地址
    struct  addrinfo    *ailist;
    struct  addrinfo    hint;
    
    int                 err;

    // 防止报警
    ailist  = NULL;

    memset(&hint, 0x00, sizeof(hint));
    hint.ai_flags = AI_CANONNAME;
    hint.ai_family = AF_INET;

    hint.ai_socktype = SOCK_DGRAM;
    hint.ai_protocol = 0;

    hint.ai_addrlen = 0;
    hint.ai_canonname = NULL;
    hint.ai_addr = NULL;
    hint.ai_next = NULL;

    if ((err = getaddrinfo(ip, to_string(port).c_str(), &hint, &ailist)) != 0)
    {
        THROW(CSocketException, ERR_SOCKET_TERMINAL, "net_connect getaddrinfo error: %s\n", gai_strerror(err));
    }

    bytes = send_to(fd, msg, len, (struct sockaddr * )ailist->ai_addr, ailist->ai_addrlen, flags);
    
    freeaddrinfo(ailist);

    return bytes;
}

ssize_t CSocketOpt::send_to(int fd, const void * msg, size_t len,
                const struct sockaddr * to, socklen_t tolen,
                int flags /* = 0 */ )
{
    ssize_t bytes;

    if (!is_ok(fd))
    {
        THROW(CSocketException, ERR_IPC_ERROR, "read error: invalid socket");
    }

    if ((bytes =::sendto(fd, msg, len, flags, to, tolen)) < 0) 
    {
        THROW(CSocketException, ERR_IPC_ERROR, "sendto error: invalid socket");
    }

    return bytes;
}

ssize_t CSocketOpt::write(int fd, const void *buf, size_t count)
{
    if (!is_ok(fd))
    {
        THROW(CSocketException, ERR_IPC_ERROR, "write error: invalid socket");
    }

    ssize_t n;

    n = ::write(fd, buf, count);
        
    if (n < 0)
    {
        if (terminal_error(errno))
        {
            THROW(CSocketException, ERR_IPC_ERROR, "write %d: %s", fd, strerror(errno));
        }
    }

    return n;
}

void CSocketOpt::get_peer_addr(int fd, std::string& ip, int & port)
{
    // 临时缓存给inet_ntop用
    char buf[20] = {0};

    struct sockaddr_in peer_address;
    int address_len = sizeof(peer_address);
    memset(&peer_address, 0x00, sizeof(peer_address));
    if (getpeername(fd, (struct sockaddr *)&peer_address, (socklen_t *)&address_len) < 0)
    {
        THROW(CSocketException, ERR_SOCKET_TERMINAL, "getpeername error: %s", strerror(errno));   
    }

    ip = inet_ntop(AF_INET, &peer_address.sin_addr, buf, 256);
    port = ntohs(peer_address.sin_port);
}

void CSocketOpt::get_sock_addr(int fd, std::string& ip, int & port)
{
    // 临时缓存给inet_ntop用
    char buf[20] = {0};

    struct sockaddr_in peer_address;
    int address_len = sizeof(peer_address);
    memset(&peer_address, 0x00, sizeof(peer_address));
    if (getpeername(fd, (struct sockaddr *)&peer_address, (socklen_t *)&address_len) < 0)
    {
        THROW(CSocketException, ERR_SOCKET_TERMINAL, "getpeername error: %s", strerror(errno));   
    }

    ip = inet_ntop(AF_INET, &peer_address.sin_addr, buf, 256);
    port = ntohs(peer_address.sin_port);
}

void CSocketOpt::get_sock_opt(int fd, int optname, void *optval, socklen_t * optlen, int level /*= SOL_SOCKET*/ )
{
    if (getsockopt(fd, level, optname, optval, optlen) < 0) 
    {
        THROW(CSocketException, ERR_SOCKET_TERMINAL, "getsockopt: %s", strerror(errno));
    }

    return;
}

void CSocketOpt::set_sock_opt(int fd, int optname, const void *optval, socklen_t optlen, int level /*= SOL_SOCKET*/ )
{
    if (setsockopt(fd, level, optname, optval, optlen) < 0) 
    {
        THROW(CSocketException, ERR_SOCKET_TERMINAL, "setsockopt: %s", strerror(errno));
    }
}

// 设置为非阻塞
void CSocketOpt::set_nonblock(int fd, bool flag /* = true */)
{
    int t;
	t = fcntl(fd, F_GETFL, 0);

    if (t < 0)
    {
        THROW(CSocketException, ERR_SOCKET_TERMINAL, "fcntl error: %s", strerror(errno));
    }
        
	if (flag) 
    { // set nonblock
		t |= O_NONBLOCK;
	}
	else 
    { // set block
		t &= (~O_NONBLOCK);
	}
    
	fcntl(fd, F_SETFL, t);
}

// 设置为长连接 -- 避免防火墙超时断开
/**
 * keep_alive = 1;      //开启keepalive属性
 * keep_idle = 600;     // 如该连接在600秒内没有任何数据往来,则进行探测 
 * keep_interval = 5;   // 探测时发包的时间间隔为5 秒
 * keep_count = 3;      // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
 */
void CSocketOpt::set_keep_alive(int fd, int keep_alive /* = 1 */, int keep_idle /* = 600 */, 
    int keep_interval /* = 5 */, int keep_count /* = 3 */)
{
    set_sock_opt(fd, SO_KEEPALIVE, (void *)&keep_alive, sizeof(keep_alive), SOL_SOCKET);
    set_sock_opt(fd, TCP_KEEPIDLE, (void *)&keep_idle, sizeof(keep_idle), SOL_TCP);
    set_sock_opt(fd, TCP_KEEPINTVL, (void *)&keep_interval, sizeof(keep_interval), SOL_TCP);
    set_sock_opt(fd, TCP_KEEPCNT, (void *)&keep_count, sizeof(keep_count), SOL_TCP);
}

void CSocketOpt::set_reuser_addr(int fd)
{
    int reuse_addr=1;
    set_sock_opt(fd, SO_REUSEADDR, (void *) (&(reuse_addr)), sizeof(reuse_addr), SOL_SOCKET);
}

bool CSocketOpt::check_tcp_established(int fd)
{   
    if (fd == INVALID_SOCKET)
    {
        return false;
    }
    
    //检测状态
    struct tcp_info tcpinfo;
    int tcp_info_length = sizeof(tcpinfo);
    getsockopt(fd,IPPROTO_TCP,TCP_INFO,&tcpinfo,(socklen_t*)&tcp_info_length);
    
    return tcpinfo.tcpi_state == TCP_ESTABLISHED;
}

// 判断socket是否为致命错误
// EAGAIN等
int CSocketOpt::terminal_error(int error_no)
{
    if (error_no == EWOULDBLOCK || error_no == ECONNABORTED
        || error_no == EPROTO || error_no == EINTR 
        || error_no == EAGAIN || error_no == EINPROGRESS)
    {
        return false;
    }

    return true;
}

bool CSocketOpt::check_tcp_ok(int fd)
{
    //检测状态
    struct tcp_info tcpinfo;
    int tcp_info_len = sizeof(tcpinfo);
    getsockopt(fd, IPPROTO_TCP,TCP_INFO, &tcpinfo, (socklen_t*)&tcp_info_len);
    if (tcpinfo.tcpi_state != TCP_ESTABLISHED)
    {
        return false;        
    }

    return true;
}

bool CSocketOpt::is_ok(int fd)
{
    return fd != INVALID_SOCKET;
}

// 解析ip:port 形式
void CSocketOpt::parse_host(const char * address, std::string& ip, int& port)
{
    const char * p = address;
    const char * index = NULL;
    int space_len = 0;
    
    static const char* SPET_CHAR = ":";

    // 去掉前空格
    while(isspace(*p))
    {
        ++p;
    }

    if ((index = strstr(p, SPET_CHAR)) == NULL)
    {
        THROW(CSocketException, ERR_SOCKET_TERMINAL, "address error: %s", address);
    }

    // 计算后空格长度
    for (const char * t = index - 1; t > p && isspace(*t); --t) {
        ++space_len;
    }
        
    ip = std::string(p, index - p - space_len);
        
    // 去掉前空格
    while(isspace(*p))
    {
        ++p;
    }
    
    port = atoi(index + 1);
}

unsigned long int CSocketOpt::get_hex_ip()
{
    int fd, intrface;
    struct ifreq buf[16];
    struct ifconf ifc; 
    unsigned long int hex_ip = 0;
    
    if((fd = socket (AF_INET, SOCK_DGRAM, 0)) >= 0) 
    {
        ifc.ifc_len = sizeof buf;
        ifc.ifc_buf = (caddr_t) buf;
        if (!ioctl(fd, SIOCGIFCONF, (char*)&ifc)) 
        {
            intrface = ifc.ifc_len / sizeof(struct ifreq);
            while(intrface-- > 0)
            {
                if (!(ioctl (fd, SIOCGIFADDR, (char *) &buf[intrface])))
                {
                    hex_ip = inet_addr(inet_ntoa(((struct sockaddr_in*)(&buf[intrface].ifr_addr))->sin_addr));   
                    if (hex_ip != 0 && hex_ip != 0x0100007f)
                    {
                        // 避免取到0.0.0.0或者127.0.0.1
                        break;
                    }
                }
            }
        }
        close (fd);
    }

    return hex_ip;
}

// 16进制ip转成xxx.xxx.xxx.xxx的格式
char * CSocketOpt::hexIP_to_normal(unsigned long int hex_ip, char * normal_ip)
{
    // 转换为.格式
    unsigned char * pIp = (unsigned char*)(&hex_ip);
    sprintf(normal_ip, "%u.%u.%u.%u", pIp[0], pIp[1], pIp[2], pIp[3]);
    
    return normal_ip;
}

char * CSocketOpt::get_local_ip(char * ip)
{
    return hexIP_to_normal(get_hex_ip(), ip);
}

ssize_t CSocketOpt::read_all(int fd, void * buffer, size_t count, int time_out)
{
    if (!is_ok(fd))
    {
        THROW(CSocketException, ERR_IPC_ERROR, "read error: invalid socket");
    }
    
    int read_count = count;
    int total_read = 0;
    int this_read = 0;
    
    do
    {
        fd_set rset;
        struct timeval tv;

        FD_ZERO(&rset);
        FD_SET(fd, &rset);
        tv.tv_sec = time_out;
        tv.tv_usec = 0;

        int n = select(fd + 1, &rset, NULL, NULL, &tv);
        if (n == 0) 
        {
            THROW(CSocketException, ERR_SOCKET_TIME_OUT, "read time out: %d", time_out);
        }
        else if (n < 0) 
        {
            if (terminal_error(errno))
            {
                THROW(CSocketException, ERR_SOCKET_TERMINAL, "select error: %s", strerror(errno));
            }
        }

        this_read = read(fd, (char *)buffer + total_read, read_count - total_read);
        
        total_read += this_read;
    }while(total_read < read_count);
    
    if (total_read != read_count)
    {
        THROW(CSocketException, ERR_SOCKET_TERMINAL, "read errror, total: %d != expect: %d", total_read, read_count);
    }
    
    return total_read;
}

ssize_t CSocketOpt::read_relay_packet(int fd, void * buffer, int length, int time_out)
{
    int packet_len = 0;
    CSocketOpt::read_all(fd, &packet_len, sizeof(int), time_out);
    if (packet_len > length)
    {
        THROW(CSocketException, ERR_SYS_UNKNOWN, 
              "too big packet length:%d, recv buffer length:%d", packet_len, length);
    }

    return CSocketOpt::read_all(fd, buffer, packet_len, time_out);
}

ssize_t CSocketOpt::write_all(int fd, const void *buffer, size_t count, int time_out)
{
    if (!is_ok(fd))
    {
        THROW(CSocketException, ERR_IPC_ERROR, "read error: invalid socket");
    }
    
    int write_count = count;
    int total_write = 0;
    int this_write = 0;
    
    do
    {
        fd_set wset;
        struct timeval tv;

        FD_ZERO(&wset);
        FD_SET(fd, &wset);
        tv.tv_sec = time_out;
        tv.tv_usec = 0;

        int n = select(fd + 1, NULL, &wset, NULL, &tv);
        if (n == 0) 
        {
            THROW(CSocketException, ERR_SOCKET_TIME_OUT, "read time out: %d", time_out);
        }
        else if (n < 0) 
        {
            if (terminal_error(errno))
            {
                THROW(CSocketException, ERR_SOCKET_TERMINAL, "select error: %s", strerror(errno));
            }
        }

        this_write = write(fd, (char *)buffer + total_write, write_count - total_write);
        
        total_write += this_write;
    }while(total_write < write_count);
    
    if (total_write != write_count)
    {
        THROW(CSocketException, ERR_SOCKET_TERMINAL, "write errror, total: %d != expect: %d", total_write, write_count);
    }
    
    return total_write;
}

ssize_t CSocketOpt::write_relay_packet(int fd, const void *buffer, int length, int time_out)
{
    int packet_len = length;

    CSocketOpt::write_all(fd, &packet_len, sizeof(int), time_out) ;

    return   CSocketOpt::write_all(fd, buffer, packet_len, time_out); 
}
