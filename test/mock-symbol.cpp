#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>

#include <string.h>
#include <stdio.h>
#include "mock-symbol.hpp"
#define __FUNC__ (__func__ + 7)

/******************************************************************************
 * c++ mock implementation 
 *****************************************************************************/
static mockSymbol sym;

void mockSymbol::setReturn(std::string symbol, int value)
{
    return_map[symbol].push_back(value);
}

int mockSymbol::willReturn(std::string symbol)
{
    std::vector<int> vec;
    int vec_len = return_map[symbol].size();
    int result = 0;
    if(vec_len != 0){
	result = return_map[symbol].front();
    }
    return_map[symbol].pop_back();
    return result;
}

void mockSymbol::setSymbol(
	std::string symbol, 
	unsigned char* buf,
	size_t len)
{
    unsigned char *info = (unsigned char*)calloc(len, sizeof(unsigned char));
    memcpy(info, buf, len);
    symbol_map[symbol].push_back(info);
}

void mockSymbol::getSymbol(
	std::string symbol, 
	unsigned char* buf,
	size_t len)
{
    if(symbol_map[symbol].size() == 0){
	return;
    }
    memcpy(buf, symbol_map[symbol].front(), len);
    free(symbol_map[symbol].front());
    symbol_map[symbol].front() = NULL;
    symbol_map[symbol].erase(symbol_map[symbol].begin());
}

/******************************************************************************
 * c interface functions
 *****************************************************************************/
mockSymbol *mockSymbol_init(void)
{
    return &sym;
}

extern "C" int __wrap_socket(int domain, int type, int protocol)
{
    return sym.willReturn(__FUNC__);
}

extern "C" int __wrap_connect(int sockfd, const struct sockaddr *addr,
	socklen_t addrlen)
{
    return sym.willReturn(__FUNC__);
}

extern "C" int __wrap_write(int fd, const void *buf, size_t count)
{
    sym.setSymbol(__FUNC__, (unsigned char *)buf, count);

    return count;
}

extern "C" ssize_t __wrap_read(int fd, void *buf, size_t count)
{
    sym.getSymbol(__FUNC__, (unsigned char *)buf, count);
    return sym.willReturn(__FUNC__);
}
