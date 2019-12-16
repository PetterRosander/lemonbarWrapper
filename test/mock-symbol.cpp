#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>

#include <string.h>
#include <stdlib.h>
#include "mock-symbol.hpp"
#define __FUNC__ (__func__ + 7)

/******************************************************************************
 * c++ mock implementation 
 *****************************************************************************/
static mockSymbol *sym = NULL;

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
    return_map[symbol].erase(return_map[symbol].begin());
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
    symbol_map[symbol].erase(symbol_map[symbol].begin());
}

int mockSymbol::clearSymbol()
{
    int uncheckedSymbols = 0;
    for(auto const& x :symbol_map){
	std::vector<unsigned char*> vec = x.second;
	int size = vec.size();
	uncheckedSymbols += size;
	for(auto &value: vec){
	    free(value);
	    value = NULL;
	}
    }
    return uncheckedSymbols;
}

/******************************************************************************
 * c interface functions
 *****************************************************************************/
void mockSymbol_init(mockSymbol *mock)
{
    sym = mock;
}

extern "C" int __mock_socket(int domain, int type, int protocol)
{
    return sym->willReturn(__FUNC__);
}

extern "C" int __mock_connect(int sockfd, const struct sockaddr *addr,
	socklen_t addrlen)
{
    return sym->willReturn(__FUNC__);
}

extern "C" int __mock_write(int fd, const void *buf, size_t count)
{
    sym->setSymbol(__FUNC__, (unsigned char *)buf, count);

    return count;
}

extern "C" ssize_t __mock_read(int fd, void *buf, size_t count)
{
    sym->getSymbol(__FUNC__, (unsigned char *)buf, count);
    return sym->willReturn(__FUNC__);
}
