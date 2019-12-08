/******************************************************************************
 * PreProcessor directive
 *****************************************************************************/
#include "workspace.h"
#include <string>

#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include "mock-symbol.hpp"



TEST_CASE( "Test setup", "[Workspace]" ) 
{
    struct workspace *ws = NULL;
    mockSymbol mock;
    mockSymbol_setCurrentSymbol(&mock);


    std::string socket_path = "./some-socket";
    REQUIRE( workspace_init(ws, (char *)socket_path.c_str()) == -1 );
}
       
