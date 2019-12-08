/******************************************************************************
 * PreProcessor directive
 *****************************************************************************/
#include <string>
#include <vector>

#include "catch2/catch.hpp"
#include "mock-symbol.hpp"

#define __WORKSPACE__
#include "workspace.h"


TEST_CASE( "Test setup", "[Workspace]" ) 
{
    struct workspace *ws = NULL;
    mockSymbol *mock;
    mock = mockSymbol_init();

    mock->will_return("socket", 123);

    std::vector <int> vec = mock->return_map["socket"];

    std::string socket_path = "./some-socket";

    REQUIRE( (ws = workspace_init((char *)socket_path.c_str())) != NULL );
    printf("%p\n", (void*)ws->internal);
    REQUIRE( workspace_destroy(ws) == 0 );
}
       
