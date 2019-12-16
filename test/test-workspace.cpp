/******************************************************************************
 * PreProcessor directive
 *****************************************************************************/
#include <string>
#include <vector>

#include "catch2/catch.hpp"

#define __WORKSPACE__
#include "workspace.h"
#include "task-runner.h"

#define MOCK __MOCK__WORKSPACE__
#include "mock-symbol.hpp"

#include <string.h>

struct workspaceTest {
    struct taskRunner task;
    struct workspace  *ws;
};

void setup(void **state)
{
    INIT_MOCK();
    std::string path = "./path";
    struct workspaceTest *test = 
	(struct workspaceTest *)calloc(1, sizeof(struct workspaceTest));
    test->ws = workspace_init((char *)path.c_str());
    *state = test;
}

int teardown(void **state)
{
    struct workspaceTest *test = (struct workspaceTest *)*state;
    workspace_destroy(test->ws);
    free(test);
    int size = CLEAR_SYMBOLS();
    return size;
}



TEST_CASE( "Unittest - setting up unix socket and connecting to i3",  
  	   "[Workspace]" )
{
    struct workspaceTest *test = NULL;
    setup((void **)&test);

    int fdMock      = 10;
    int connectMock = 0;
    SET_MOCK_SYMBOL(socket, fdMock);
    SET_MOCK_SYMBOL(connect, connectMock);


    workspace_setupSocket(&test->task, test->ws);
    REQUIRE(test->ws->fd == fdMock);
    REQUIRE(test->task.exitStatus == 0);
    REQUIRE(test->task.nextTask == workspace_subscribeWorkspace);

    REQUIRE( teardown((void **)&test) == 0);
}

TEST_CASE( "Unittest - Make sure we subscribe correctly to i3",  
  	   "[Workspace]" )
{
    struct workspaceTest *test = NULL;
    setup((void **)&test);

    SET_MOCK_SYMBOL(read, -1);

    workspace_subscribeWorkspace(&test->task, test->ws);

    unsigned char buf[20] = {0};
    GET_MOCK_SYMBOL(write, buf, 14);
    REQUIRE( memcmp((const void *)buf, (const void *)"i3-ipc\x15\x0\x0\x0\x2\x0\x0", 14)  == 0);
    GET_MOCK_SYMBOL(write, buf, 16);
    REQUIRE( memcmp((const void *)buf, (const void *)"[ \"workspace\" ]", 16)  == 0);

    REQUIRE(teardown((void **)&test) == 0);
}
