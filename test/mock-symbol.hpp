#ifndef _MOCK_SYMBOL_H_
#define _MOCK_SYMBOL_H_
#include <vector>
#include <map>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
class mockSymbol {
    public:
	int willReturn(std::string);
	int clearSymbol();
	void setReturn(std::string, int);
	void setSymbol(std::string symbol, unsigned char* buf, size_t len);
	void getSymbol(std::string symbol, unsigned char* buf, size_t len);

    private:
	std::vector<std::string> writeBuffer;
	std::map<std::string, std::vector<int> > return_map;
	std::map<std::string, std::vector<unsigned char*> > symbol_map;

};

mockSymbol *mockSymbol_init(void);

static mockSymbol *MOCK = NULL;

#define INIT_MOCK() \
    mockSymbol *mock = mockSymbol_init();\
    MOCK = mock;

#define SET_MOCK_SYMBOL(symbol, value) \
    MOCK->setReturn(#symbol, value);

#define GET_MOCK_SYMBOL(symbol, value, len) \
    MOCK->getSymbol(#symbol, (unsigned char*)value, len);

#define CLEAR_SYMBOLS() \
     MOCK->clearSymbol();\

#endif
