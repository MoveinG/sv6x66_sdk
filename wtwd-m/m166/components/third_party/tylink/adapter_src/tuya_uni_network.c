#define __UNI_NETWORK_GLOBALS
#include "tuya_uni_network.h"
//#include "uni_system.h"

#define ty_net_printf
#define CANONNAME_MAX 128

typedef struct NETWORK_ERRNO_TRANS {
    INT_T sys_err;
    INT_T priv_err;
}NETWORK_ERRNO_TRANS_S;

CONST NETWORK_ERRNO_TRANS_S unw_errno_trans[]= {
    {EINTR,UNW_EINTR},
    {EBADF,UNW_EBADF},
    {EAGAIN,UNW_EAGAIN},
    {EFAULT,UNW_EFAULT},
    {EBUSY,UNW_EBUSY},
    {EINVAL,UNW_EINVAL},
    {ENFILE,UNW_ENFILE},
    {EMFILE,UNW_EMFILE},
    {ENOSPC,UNW_ENOSPC},
    {EPIPE,UNW_EPIPE},
    {EWOULDBLOCK,UNW_EWOULDBLOCK},
    {ENOTSOCK,UNW_ENOTSOCK},
    {ENOPROTOOPT,UNW_ENOPROTOOPT},
    {EADDRINUSE,UNW_EADDRINUSE},
    {EADDRNOTAVAIL,UNW_EADDRNOTAVAIL},
    {ENETDOWN,UNW_ENETDOWN},
    {ENETUNREACH,UNW_ENETUNREACH},
    {ENETRESET,UNW_ENETRESET},
    {ECONNRESET,UNW_ECONNRESET},
    {ENOBUFS,UNW_ENOBUFS},
    {EISCONN,UNW_EISCONN},
    {ENOTCONN,UNW_ENOTCONN},
    {ETIMEDOUT,UNW_ETIMEDOUT},
    {ECONNREFUSED,UNW_ECONNREFUSED},
    {EHOSTDOWN,UNW_EHOSTDOWN},
    {EHOSTUNREACH,UNW_EHOSTUNREACH},
    {ENOMEM ,UNW_ENOMEM},
    {EMSGSIZE,UNW_EMSGSIZE}
};
    
/***********************************************************
*  Function: unw_get_errno
*  Input: none
*  Output: none
*  Return: UNW_ERRNO_T
***********************************************************/
UNW_ERRNO_T tuya_unw_get_errno(VOID)
{
    INT_T i = 0;

    INT_T sys_err = errno;

    for(i = 0;i < CNTSOF(unw_errno_trans);i++) {
        if(unw_errno_trans[i].sys_err == sys_err) {
            return unw_errno_trans[i].priv_err;
        }
    }

    return UNW_FAIL;
}

/***********************************************************
*  Function: unw_select
*  Input: maxfdp ms_timeout
*  Output: readfds writefds errorfds
*  Return: as same as the socket return
***********************************************************/
INT_T tuya_unw_select(IN CONST INT_T maxfd,INOUT UNW_FD_SET *readfds,INOUT UNW_FD_SET *writefds,\
               OUT UNW_FD_SET *errorfds,IN CONST UINT_T ms_timeout)
{
    if(maxfd <= 0) {
        return UNW_FAIL;
    }

    struct timeval *tmp = NULL;
    struct timeval timeout = {ms_timeout/1000, (ms_timeout%1000)*1000};
    if(0 != ms_timeout) {
        tmp = &timeout;
    }else {
        tmp = NULL;
    }

    return select(maxfd,readfds,writefds,errorfds,tmp);
}
    
/***********************************************************
*  Function: unw_get_nonblock
*  Input: fd 
*  Output: none
*  Return: <0  fail
*          >0  non block
*          ==0 block
***********************************************************/
INT_T tuya_unw_get_nonblock(IN CONST INT_T fd)
{
    if( fd < 0 ) {
        return -1;
    }

    if((fcntl(fd, F_GETFL, 0) & O_NONBLOCK) != O_NONBLOCK) {
        return 0;
    }

    if(errno == EAGAIN || errno == EWOULDBLOCK) {
        return 1;
    }

    return 0 ;
}
    
/***********************************************************
*  Function: unw_get_nonblock
*  Input: fd block
*  Output: none
*  Return: UNW_FAIL/UNW_SUCCESS
***********************************************************/
INT_T tuya_unw_set_block(IN CONST INT_T fd,IN CONST BOOL_T block)
{
    if( fd < 0 ) {
        return UNW_FAIL;
    }

    INT_T flags = fcntl(fd, F_GETFL, 0);
    if(TRUE == block) {
        flags &= (~O_NONBLOCK);
    }else {
        flags |= O_NONBLOCK;
    }

    if (fcntl(fd,F_SETFL,flags) < 0) {
        return UNW_FAIL;
    }

    return UNW_SUCCESS;
}

/***********************************************************
*  Function: unw_close
*  Input: fd
*  Output: none
*  Return: as same as the socket return
***********************************************************/
INT_T tuya_unw_close(IN CONST INT_T fd)
{
    return close(fd);
}

/***********************************************************
*  Function: unw_shutdown
*  Input: fd how
*  Output: none
*  Return: as same as the socket return
************************************************************/
INT_T tuya_unw_shutdown(IN CONST INT_T fd,IN CONST INT_T how)
{
    return shutdown(fd,how);
}
    
/***********************************************************
*  Function: unw_socket_create
*  Input: type
*  Output: none
*  Return: as same as the socket return
************************************************************/
INT_T tuya_unw_socket_create(IN CONST UNW_PROTOCOL_TYPE type)
{
    ty_net_printf("%s type=%d\n", __func__, type);

    INT_T fd = -1;

    if(PROTOCOL_TCP == type) {
        fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    }else {
        fd = socket(AF_INET, SOCK_DGRAM,0);
    }

    return fd;
}

/***********************************************************
*  Function: unw_connect
*  Input: fd addr port
*  Output: none
*  Return: as same as the socket return
************************************************************/
INT_T tuya_unw_connect(IN CONST INT_T fd,IN CONST UNW_IP_ADDR_T addr,IN CONST USHORT_T port)
{
    ty_net_printf("%s fd=%d, addr=0x%x, port=%d\n", __func__, fd, addr, port);

    struct sockaddr_in sock_addr;
    USHORT_T tmp_port = port;
    UNW_IP_ADDR_T tmp_addr = addr;

    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = UNI_HTONS(tmp_port);
    sock_addr.sin_addr.s_addr = UNI_HTONL(tmp_addr);
    
    return connect(fd, (struct sockaddr*)&sock_addr, sizeof(struct sockaddr_in));
}

/***********************************************************
*  Function: unw_connect_raw
*  Input: fd addr port
*  Output: none
*  Return: as same as the socket return
************************************************************/
INT_T tuya_unw_connect_raw(IN CONST INT_T fd, void *p_socket_addr, IN CONST INT_T len)
{
    return connect(fd, (struct sockaddr *)p_socket_addr, len);
}

/***********************************************************
*  Function: unw_bind
*  Input: fd
*         addr-> if(addr == 0) then bind src ip by system select
*         port
*  Output: none
*  Return: as same as the socket return
************************************************************/
INT_T tuya_unw_bind(IN CONST INT_T fd,IN CONST UNW_IP_ADDR_T addr,IN CONST USHORT_T port)
{
    ty_net_printf("%s fd=%d, addr=0x%x, port=%d\n", __func__, fd, addr, port);

    if( fd < 0 ) {
        return UNW_FAIL;
    }
    
    USHORT_T tmp_port = port;
    UNW_IP_ADDR_T tmp_addr = addr;

    struct sockaddr_in sock_addr;
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = UNI_HTONS(tmp_port);
    sock_addr.sin_addr.s_addr = UNI_HTONL(tmp_addr);

    return bind(fd,(struct sockaddr*)&sock_addr, sizeof(struct sockaddr_in));
}

/***********************************************************
*  Function: unw_listen
*  Input: fd
*         backlog
*  Output: none
*  Return: as same as the socket return
************************************************************/
INT_T tuya_unw_listen(IN CONST INT_T fd,IN CONST INT_T backlog)
{
    if( fd < 0 ) {
        return UNW_FAIL;
    }

    return listen(fd,backlog);
}

/***********************************************************
*  Function: unw_accept
*  Input: fd
*  Output: addr port
*  Return: as same as the socket return
************************************************************/
INT_T tuya_unw_accept(IN CONST INT_T fd,OUT UNW_IP_ADDR_T *addr,OUT USHORT_T *port)
{
    if( fd < 0 ) {
        return UNW_FAIL;
    }

    struct sockaddr_in sock_addr;
    socklen_t len = sizeof(struct sockaddr_in);
    INT_T cfd = accept(fd, (struct sockaddr *)&sock_addr,&len);
    if(cfd < 0) {
        return UNW_FAIL;
    }

    if(addr) {
        *addr = UNI_NTOHL((sock_addr.sin_addr.s_addr));
    }

    if(port) {
        *port = UNI_NTOHS((sock_addr.sin_port));
    }

    return cfd;
}

/***********************************************************
*  Function: unw_send
*  Input: fd buf nbytes
*  Output: none
*  Return: as same as the socket return
***********************************************************/
INT_T tuya_unw_send(IN CONST INT_T fd, IN CONST VOID *buf, IN CONST UINT_T nbytes)
{
    if( fd < 0 ) {
        return UNW_FAIL;
    }
    
    return send(fd,buf,nbytes,0);
}

/***********************************************************
*  Function: unw_send_to
*  Input: fd buf nbytes addr port
*  Output: none
*  Return: as same as the socket return
***********************************************************/
INT_T tuya_unw_send_to(IN CONST INT_T fd, IN CONST VOID *buf, IN CONST UINT_T nbytes,\
                IN CONST UNW_IP_ADDR_T addr,IN CONST USHORT_T port)
{
    USHORT_T tmp_port = port;
    UNW_IP_ADDR_T tmp_addr = addr;

    struct sockaddr_in sock_addr;
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = UNI_HTONS(tmp_port);
    sock_addr.sin_addr.s_addr = UNI_HTONL(tmp_addr);

    return sendto(fd,buf,nbytes,0,(struct sockaddr *)&sock_addr,SIZEOF(sock_addr));
}

/***********************************************************
*  Function: unw_recv
*  Input: sockFd nbytes
*  Output: buf
*  Return: as same as the socket return
***********************************************************/
INT_T tuya_unw_recv(IN CONST INT_T fd, OUT VOID *buf, IN CONST UINT_T nbytes)
{
    if( fd < 0 ) {
        return UNW_FAIL;
    }

    return recv(fd,buf,nbytes,0);
}

/***********************************************************
*  Function: unw_recv_nd_size
*  Input: fd buf_size nd_size
*  Output: buf
*  Return: < 0 error
***********************************************************/
INT_T tuya_unw_recv_nd_size(IN CONST INT_T fd, OUT VOID *buf, \
                     IN CONST UINT_T buf_size,IN CONST UINT_T nd_size)
{
    if(NULL == buf || \
       buf_size < nd_size) {
        return -1;
    }

    UINT_T rd_size = 0;
    INT_T ret = 0;

    while(rd_size < nd_size) {
        ret = recv(fd,((BYTE_T *)buf+rd_size),nd_size-rd_size,0);
        if(ret <= 0) {
            UNW_ERRNO_T err = unw_get_errno();
            if(UNW_EWOULDBLOCK == err || \
               UNW_EINTR == err || \
               UNW_EAGAIN == err) {
                SystemSleep(10);
                continue;
            }

            break;
        }

        rd_size += ret;
    }

    if(rd_size < nd_size) {
        return -2;
    }

    return rd_size;
}


/***********************************************************
*  Function: unw_recvfrom
*  Input: fd nbytes 
*  Output: buf addr port
*  Return: as same as the socket return
***********************************************************/
INT_T tuya_unw_recvfrom(IN CONST INT_T fd,OUT VOID *buf,IN CONST UINT_T nbytes,\
                 OUT UNW_IP_ADDR_T *addr,OUT USHORT_T *port)
{
    struct sockaddr_in sock_addr;
    socklen_t addr_len = 0;
    int ret = recvfrom(fd,buf,nbytes,0,(struct sockaddr *)&sock_addr,&addr_len);
    if(ret <= 0) {
        return ret;
    }

    if(addr) {
        *addr = UNI_NTOHL(sock_addr.sin_addr.s_addr);
    }

    if(port) {
        *port = UNI_NTOHS(sock_addr.sin_port);
    }
    
    return ret;
}

/***********************************************************
*  Function: unw_set_timeout
*  Input: fd ms_timeout trans_type
*  Output: none
*  Return: UNW_FAIL/UNW_SUCCESS
***********************************************************/
INT_T tuya_unw_set_timeout(IN CONST INT_T fd,IN CONST INT_T ms_timeout,\
                    IN CONST UNW_TRANS_TYPE_E type)
{    
    if( fd < 0 ) {
        return UNW_FAIL;
    }

    struct timeval timeout = {ms_timeout/1000, (ms_timeout%1000)*1000};
    int optname = ((type == TRANS_RECV) ? SO_RCVTIMEO:SO_SNDTIMEO);

    if(0 != setsockopt(fd, SOL_SOCKET, optname, (char *)&timeout, sizeof(timeout))) {
        return UNW_FAIL;
    }
    
    return UNW_SUCCESS;
}

/***********************************************************
*  Function: unw_set_bufsize
*  Input: fd ms_timeout trans_type
*  Output: none
*  Return: UNW_FAIL/UNW_SUCCESS
***********************************************************/
INT_T tuya_unw_set_bufsize(IN CONST INT_T fd,IN CONST INT_T buf_size,\
                    IN CONST UNW_TRANS_TYPE_E type)
{
    if( fd < 0 ) {
        return UNW_FAIL;
    }

    int size = buf_size;
    int optname = ((type == TRANS_RECV) ? SO_RCVBUF:SO_SNDBUF);

    if(0 != setsockopt(fd, SOL_SOCKET, optname, (char *)&size, sizeof(size))) {
        return UNW_FAIL;
    }

    return UNW_SUCCESS;
}

/***********************************************************
*  Function: unw_set_reuse
*  Input: fd
*  Output: none
*  Return: UNW_FAIL/UNW_SUCCESS
***********************************************************/
INT_T tuya_unw_set_reuse(IN CONST INT_T fd)
{
    if( fd < 0 ) {
        return UNW_FAIL;
    }

    int flag = 1;
    if(0 != setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,(const char*)&flag,sizeof(int))) {
        return UNW_FAIL;
    }

    return UNW_SUCCESS;
}

/***********************************************************
*  Function: unw_disable_nagle
*  Input: fd
*  Output: none
*  Return: UNW_FAIL/UNW_SUCCESS
***********************************************************/
INT_T tuya_unw_disable_nagle(IN CONST INT_T fd)
{
    if( fd < 0 ) {
        return UNW_FAIL;
    }

    int flag = 1;
    if(0 != setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,(const char*)&flag,sizeof(int))) {
        return UNW_FAIL;
    }

    return UNW_SUCCESS;
}

/***********************************************************
*  Function: unw_set_boardcast
*  Input: fd
*  Output: none
*  Return: UNW_FAIL/UNW_SUCCESS
***********************************************************/
INT_T tuya_unw_set_boardcast(IN CONST INT_T fd)
{
    if( fd < 0 ) {
        return UNW_FAIL;
    }

    int flag = 1;
    if(0 != setsockopt(fd,SOL_SOCKET,SO_BROADCAST,(const char*)&flag,sizeof(int))) {
        return UNW_FAIL;
    }

    return UNW_SUCCESS;
}

/***********************************************************
*  Function: unw_gethostbyname
*  Input: domain
*  Output: addr
*  Return: UNW_FAIL/UNW_SUCCESS
***********************************************************/
INT_T tuya_unw_gethostbyname(IN CONST CHAR_T *domain,OUT UNW_IP_ADDR_T *addr)
{
	struct hostent* h;
	if((h = gethostbyname(domain)) == NULL) {
		return UNW_FAIL;
    }

    ty_net_printf("%s domain=%s, addr=0x%x\n", __func__, domain);

    *addr = UNI_NTOHL(((struct in_addr *)(h->h_addr_list[0]))->s_addr);

    return UNW_SUCCESS;
}

/***********************************************************
*  Function: unw_getaddrinfo
*  Input: hostname, service, type
*  Output: buff addrinfo
*  Return: UNW_FAIL/UNW_SUCCESS
***********************************************************/
INT_T tuya_unw_getaddrinfo(IN CONST CHAR_T *hostname, IN CONST CHAR_T *service,
                           IN CONST UNW_PROTOCOL_TYPE type, OUT ADDRINFO** res)
{
	struct addrinfo hints;

	memset( &hints, 0, sizeof( hints ) );
	
	#if 0
	if( hostname == NULL ) {
        hints.ai_flags = AI_PASSIVE;
    }
    #endif

    hints.ai_family = AF_INET;
    hints.ai_socktype = type == PROTOCOL_UDP ? SOCK_DGRAM : SOCK_STREAM;
    hints.ai_protocol = type == PROTOCOL_UDP ? IPPROTO_UDP : IPPROTO_TCP;

	if(getaddrinfo(hostname, service, &hints, res) != 0 )
		return(UNW_FAIL);

    return UNW_SUCCESS;
}

/***********************************************************
*  Function: unw_getsockname
*  Input: fd
*  Output: len
*  Return: UNW_FAIL/UNW_SUCCESS
***********************************************************/
INT_T tuya_unw_getsockname(IN INT_T fd, OUT INT_T *len)
{
    struct sockaddr local_addr;
	if(getsockname(fd, &local_addr, (socklen_t *)len) != 0)
	{
		return UNW_FAIL;
    }

    return UNW_SUCCESS;
}

UNW_IP_ADDR_T tuya_unw_str2addr(IN CHAR_T *ip_str)
{
    if(ip_str == NULL)
    {
        return 0xFFFFFFFF;
    }
    UNW_IP_ADDR_T addr1 = inet_addr(ip_str);
    UNW_IP_ADDR_T addr2 = UNI_NTOHL(addr1);
    // UNW_IP_ADDR_T addr3 = inet_network(ip_str);

    return addr2;
}

/***********************************************************
*  Function: unw_set_keepalive
*  Input: alive
*  Output: len
*  Return: UNW_FAIL/UNW_SUCCESS
***********************************************************/
INT_T tuya_unw_set_keepalive(IN INT_T fd,IN CONST BOOL_T alive,\
                              IN CONST UINT_T idle,IN CONST UINT_T intr,\
                              IN CONST UINT_T cnt)
{
    if( fd < 0 ) {
        return UNW_FAIL;
    }

    INT_T ret = 0;
    int keepalive = alive;
    int keepidle = idle;
    int keepinterval = intr;
    int keepcount = cnt;

    ret |= setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive));
    ret |= setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, (void*)&keepidle , sizeof(keepidle));
    ret |= setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&keepinterval , sizeof(keepinterval));
    ret |= setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, (void *)&keepcount , sizeof(keepcount));

    ty_net_printf("%s ret=%d, fd=%d, alive=%d, idle=%d, intr=%d, cnt=%d\n", __func__, ret, fd, alive, idle, intr, cnt);

    if(0 != ret) {
        return UNW_FAIL;
    }

    return UNW_SUCCESS;
}

/***********************************************************
*  Function: unw_socket_bind
*  Input: alive
*  Output: len
*  Return: UNW_FAIL/UNW_SUCCESS
***********************************************************/
INT_T tuya_unw_socket_bind(IN INT_T fd,IN CONST CHAR_T *ip)
{
    if(NULL == ip) {
        return UNW_FAIL;
    }

    struct sockaddr_in addr_client   = {0};
    addr_client.sin_family   = AF_INET;
    addr_client.sin_addr.s_addr      = inet_addr(ip);
    addr_client.sin_port     = 0;    /// 0 表示由系统自动分配端口号

    if (0 != bind(fd,(struct sockaddr*)&addr_client,sizeof(addr_client))) {
        return UNW_FAIL;
    }

    return UNW_SUCCESS;
}


/***********************************************************
*  Function: unw_set_cloexec
*  Input: fd
*  Output: none
*  Return: UNW_FAIL/UNW_SUCCESS
***********************************************************/
INT_T tuya_unw_set_cloexec(IN CONST INT_T fd)
{

    return UNW_SUCCESS;
}


