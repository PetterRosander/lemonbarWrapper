/******************************************************************************
 * PreProcessor directive
 *****************************************************************************/
#include <string>
#include <vector>

#include "catch2/catch.hpp"
#include "mock-symbol.hpp"

#define __WORKSPACE__
#include "workspace.h"
#include "task-runner.h"

struct workspaceTest {
    struct taskRunner task;
    struct workspace  *ws;
};

void setup(void **state)
{
    std::string path = "./path";
    struct workspaceTest *test = 
	(struct workspaceTest *)calloc(1, sizeof(struct workspaceTest));
    test->ws = workspace_init((char *)path.c_str());
    *state = test;
}

void teardown(void **state)
{
    struct workspaceTest *test = (struct workspaceTest *)*state;
    workspace_destroy(test->ws);
    free(test);
}



TEST_CASE( "Unittest - setting up unix socket and connecting to i3",  
  	   "[Workspace]" )
{
    struct workspaceTest *test = NULL;
    setup((void **)&test);

    INIT_MOCK();

    int fdMock      = 10;
    int connectMock = 0;
    SET_MOCK_SYMBOL(socket, fdMock);
    SET_MOCK_SYMBOL(connect, connectMock);


    workspace_setupSocket(&test->task, test->ws);
    REQUIRE(test->ws->fd == fdMock);
    REQUIRE(test->task.exitStatus == 0);
    REQUIRE(test->task.nextTask == workspace_subscribeWorkspace);

    teardown((void **)&test);
}

TEST_CASE( "Unittest - Make sure we subsrcibe correctly to i3",  
  	   "[Workspace]" )
{
    struct workspaceTest *test = NULL;
    setup((void **)&test);

    INIT_MOCK();

    SET_MOCK_SYMBOL(read, -1);

    workspace_subscribeWorkspace(&test->task, test->ws);

    unsigned char buf[20] = {0};
    GET_MOCK_SYMBOL(write, buf, 14);
    printf("%s\n", buf);
    GET_MOCK_SYMBOL(write, buf, 16);
    printf("%s\n", buf);

    teardown((void **)&test);
}
