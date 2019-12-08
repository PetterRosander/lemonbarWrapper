#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>

#include <string.h>
#include <stdio.h>
#include "mock-symbol.hpp"

/******************************************************************************
 * c++ mock implementation 
 *****************************************************************************/
int mockSymbol::socket(int domain, int type, int protocol)
{
    std::vector <int> vec;
    int vec_len = return_map["socket"].size();
    int result = 0;
    if(vec_len != 0){
	result = return_map["socket"].front();
    }
    return_map["socket"].pop_back();
    return result;
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

ssize_t mockSymbol::read(int fd, void *buf, size_t count)
{
    strcpy((char *)buf, "Some");
    return count;
}

void mockSymbol::will_return(std::string symbol, int retVal)
{
    return_map[symbol].push_back(retVal);
}
/******************************************************************************
 * c interface functions
 *****************************************************************************/
static mockSymbol sym;

mockSymbol *mockSymbol_init(void)
{
    return &sym;
}

int socket(int domain, int type, int protocol)
{
    return sym.socket(domain, type, protocol);
}

int connect(int sockfd, const struct sockaddr *addr,
	socklen_t addrlen)
{
    return sym.connect(sockfd, addr, addrlen);
}

int write(int fd, const void *buf, size_t count)
{
    return sym.write(fd, buf, count);
}

ssize_t read(int fd, void *buf, size_t count)
{
    return sym.read(fd, buf, count);
}
