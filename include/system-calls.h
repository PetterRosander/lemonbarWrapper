#if UNIT_TEST
ssize_t __mock_read(int fd, void *buf, size_t count);
int __mock_socket(int domain, int type, int protocol);
int __mock_write(int fd, const void *buf, size_t count);
int __mock_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

#define READ(fd, buf, count) \
    __mock_read(fd, buf, count)

#define SOCKET(domain, type, protocol) \
    __mock_socket(domain, type, protocol)

#define WRITE(fd, buf, count) \
    __mock_write(fd, buf, count)

#define CONNECT(sockfd, addr, addrlen) \
    __mock_connect(sockfd, addr, addrlen)

#else /* NOT UNIT_TEST */

#define READ(fd, buf, count) \
    read(fd, buf, count)

#define SOCKET(domain, type, protocol) \
    socket(domain, type, protocol)

#define WRITE(fd, buf, count) \
    write(fd, buf, count)

#define CONNECT(sockfd, addr, addrlen) \
    connect(sockfd, addr, addrlen)

#endif /* UNIT_TEST */
