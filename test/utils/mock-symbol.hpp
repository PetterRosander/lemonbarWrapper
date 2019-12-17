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

void mockSymbol_init(mockSymbol *mock);


#define INIT_MOCK(state) \
    mockSymbol mock; \
    mockSymbol_init(&mock);\
    setup(state);

#define SET_RETURN(symbol, value) \
    mock.setReturn(#symbol, value);

#define SET_MOCK_SYMBOL(symbol, value) \
    mock.setSymbol(#symbol, (unsigned char *)value, sizeof(value));

#define GET_MOCK_SYMBOL(symbol, value, len) \
    mock.getSymbol(#symbol, (unsigned char*)value, len);

#define TEARDOWN(state, symsLeft) \
    teardown(state); \
    REQUIRE( mock.clearSymbol() == symsLeft);

#endif
