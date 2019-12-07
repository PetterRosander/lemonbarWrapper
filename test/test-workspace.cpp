#include <string>

#define CATCH_CONFIG_MAIN


#include "catch2/catch.hpp"

#include "workspace.h"

TEST_CASE( "Factorial of 0 is 1 (fail)", "[single-file]" ) {
    struct workspace *ws = NULL;
    std::string socket_path = "./some-socket";
    REQUIRE( workspace_init(ws, (char *)socket_path.c_str()) == -1 );
}
       
