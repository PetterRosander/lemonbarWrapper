#include <string>
#include <sys/types.h>
#include <sys/socket.h>

#include "mock-symbol.hpp"

/******************************************************************************
 * c++ mock implementation 
 *****************************************************************************/
int mockSymbol::socket(int domain, int type, int protocol)
{
    return 20;
}

int mockSymbol::connect(int fd, const struct sockaddr *, socklen_t)
{
    return 0;
}

int mockSymbol::write(int fd, const void *buf, size_t count)
{
    std::string _buf = (char *)buf;
    
    writeBuffer.push_back(_buf);
    return count;
}

#include <string.h>
ssize_t mockSymbol::read(int fd, void *buf, size_t count)
{
    strcpy((char *)buf, "Some");
    return count;
}
/******************************************************************************
 * c interface functions
 *****************************************************************************/
static mockSymbol *sym = NULL;

void mockSymbol_setCurrentSymbol(mockSymbol *_sym)
{
    sym = _sym;
}

int __wrap_socket(int domain, int type, int protocol)
{
    return sym->socket(domain, type, protocol);
}

int __wrap_connect(int sockfd, const struct sockaddr *addr,
	socklen_t addrlen)
{
    return sym->connect(sockfd, addr, addrlen);
}

int __wrap_write(int fd, const void *buf, size_t count)
{
    return sym->write(fd, buf, count);
}

ssize_t __wrap_read(int fd, void *buf, size_t count)
{
    return sym->read(fd, buf, count);
}
