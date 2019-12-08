#ifndef _MOCK_SYMBOL_H_
#define _MOCK_SYMBOL_H_
#include <vector>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
class mockSymbol {
    public:
	int socket(int, int, int); 
	int connect(int, const struct sockaddr *, socklen_t);
	int write(int, const void *, size_t);
	ssize_t read(int, void *, size_t);

    private:
	std::vector<std::string> writeBuffer;

};

void mockSymbol_setCurrentSymbol(mockSymbol *);
#endif
