/******************************************************************************
 * PreProcessor directive
 *****************************************************************************/
#define __WORKSPACE__
#include "workspace.h"

#include <string.h>

#include <string>
#include <vector>

#include "catch2/catch.hpp"
#include "task-runner.h"
#include "mock-symbol.hpp"
#include "workspace-data.h"


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
    INIT_MOCK((void **)&test);

    int fdMock      = 10;
    int connectMock = 0;
    SET_RETURN(socket, fdMock);
    SET_RETURN(connect, connectMock);


    workspace_setupSocket(&test->task, test->ws);
    REQUIRE(test->ws->fd == fdMock);
    REQUIRE(test->task.exitStatus == 0);
    REQUIRE(test->task.nextTask == workspace_subscribeWorkspace);

    TEARDOWN((void **)&test, 0);
}

TEST_CASE( "Unittest - Make sure we subscribe correctly to i3",  
  	   "[Workspace]" )
{
    struct workspaceTest *test = NULL;
    INIT_MOCK((void **)&test);

    test->ws->fd = 10;

    SET_RETURN(read, sizeof("i3-ipc\x10\x0\x0\x0\x2\x0\x0\x0"));
    SET_RETURN(read, sizeof("{\"success\":true}"));
    SET_MOCK_SYMBOL(read, "i3-ipc\x10\x0\x0\x0\x2\x0\x0\x0");
    SET_MOCK_SYMBOL(read, "{\"success\":true}");

    workspace_subscribeWorkspace(&test->task, test->ws);

    unsigned char buf[20] = {0};
    GET_MOCK_SYMBOL(write, buf, 14);
    REQUIRE( memcmp((const void *)buf, 
		(const void *)"i3-ipc\x15\x0\x0\x0\x2\x0\x0", 14)  == 0);
    GET_MOCK_SYMBOL(write, buf, 16);
    REQUIRE( memcmp((const void *)buf, 
		(const void *)"[ \"workspace\" ]", 16)  == 0);

    REQUIRE( test->task.nextTask == workspace_startWorkspace );

    TEARDOWN((void **)&test, 0);
}


TEST_CASE( "Unittest - Make sure we get the current workspace state correctly",  
  	   "[Workspace]" )
{
    struct workspaceTest *test = NULL;
    INIT_MOCK((void **)&test);

    test->ws->fd = 10;

    SET_RETURN(read, sizeof("i3-ipc\x01\x07\x0\x0\x1\x0\x0\x0"));
    SET_RETURN(read, sizeof(STARTWORKSPACE));
    SET_MOCK_SYMBOL(read, "i3-ipc\x07\x01\x0\x0\x1\x0\x0\x0");
    SET_MOCK_SYMBOL(read, STARTWORKSPACE);

    workspace_startWorkspace(&test->task, test->ws);
    REQUIRE (strncmp(test->ws->internal->json, STARTWORKSPACE, 263) == 0 );
    REQUIRE (test->ws->internal->lenjson == sizeof(STARTWORKSPACE) );
    free(test->ws->internal->json);
    test->ws->internal->json = NULL;


    unsigned char buf[14] = {0};
    GET_MOCK_SYMBOL(write, buf, 14);
    REQUIRE( memcmp((const void *)buf, 
		(const void *)"i3-ipc\x0\x0\x0\x0\x1\x0\x0", 14)  == 0);

    REQUIRE( test->task.nextTask == workspace_parseInitWorkspace );

    TEARDOWN((void **)&test, 0);
}

TEST_CASE( "Unittest - Make sure we parse the current workspace state correctly",  
  	   "[Workspace]" )
{
    struct workspaceTest *test = NULL;
    INIT_MOCK((void **)&test);

    test->ws->fd = 10;

    test->ws->internal->json = 
	(char *)malloc(sizeof(STARTWORKSPACE) * sizeof(char) + 1 );
    strcpy(test->ws->internal->json, STARTWORKSPACE);
    test->ws->internal->lenjson = sizeof(STARTWORKSPACE) + 1;

    workspace_parseInitWorkspace(&test->task, test->ws);

    REQUIRE( test->task.nextTask == NULL );
    REQUIRE( test->ws->json[0].focused == true);
    REQUIRE( test->ws->json[1].focused == false);
    REQUIRE( test->ws->json[0].active == true);
    REQUIRE( test->ws->json[1].active == true);
    REQUIRE( test->ws->json[0].num == 1);
    REQUIRE( test->ws->json[1].num == 2);
    REQUIRE( strcmp(test->ws->json[0].name, "1") == 0);
    REQUIRE( strcmp(test->ws->json[1].name, "2") == 0);

    TEARDOWN((void **)&test, 0);
}

TEST_CASE( "Unittest - Read a new event",  
  	   "[Workspace]" )
{
    struct workspaceTest *test = NULL;
    INIT_MOCK((void **)&test);

    test->ws->fd = 10;

    SET_RETURN(read, sizeof("i3-ipc\xd5\x0b\x0\x0\x1\x0\x0\x0"));
    SET_RETURN(read, sizeof(CHANGEFOCUS));
    SET_MOCK_SYMBOL(read, "i3-ipc\xd5\x0b\x0\x0\x1\x0\x0\x0");
    SET_MOCK_SYMBOL(read, CHANGEFOCUS);

    workspace_eventWorkspace(&test->task, test->ws);
    REQUIRE (strncmp(test->ws->internal->json, CHANGEFOCUS, 263) == 0 );
    REQUIRE (test->ws->internal->lenjson == sizeof(CHANGEFOCUS) );
    free(test->ws->internal->json);
    test->ws->internal->json = NULL;


    REQUIRE( test->task.nextTask == workspace_parseEvent );

    TEARDOWN((void **)&test, 0);
}


TEST_CASE( "Unittest - Make sure we parse a init of a new workspace correctly",  
  	   "[Workspace]" )
{
    struct workspaceTest *test = NULL;
    INIT_MOCK((void **)&test);

    test->ws->fd = 10;
    test->ws->json[0].focused = true;
    test->ws->json[1].focused = false;
    test->ws->json[0].active = true;
    test->ws->json[1].active = true;
    test->ws->json[0].num = 1;
    test->ws->json[1].num = 2;
    strcpy(test->ws->json[0].name, "1");
    strcpy(test->ws->json[1].name, "2");


    test->ws->internal->json = 
	(char *)malloc(sizeof(INITWORKSPACE) * sizeof(char));

    strcpy(test->ws->internal->json, INITWORKSPACE);
    test->ws->internal->lenjson = sizeof(INITWORKSPACE);

    workspace_parseEvent(&test->task, test->ws);

    free(test->ws->internal->json);
    REQUIRE( test->task.exitStatus == 0 );
    REQUIRE( test->task.nextTask == NULL );
    REQUIRE( test->ws->json[0].focused == false);
    REQUIRE( test->ws->json[1].focused == false);
    REQUIRE( test->ws->json[3].focused == true);
    REQUIRE( test->ws->json[0].active == true);
    REQUIRE( test->ws->json[1].active == true);
    REQUIRE( test->ws->json[3].active == true);
    REQUIRE( test->ws->json[0].num == 1);
    REQUIRE( test->ws->json[1].num == 2);
    REQUIRE( test->ws->json[3].num == 3);
    REQUIRE( strcmp(test->ws->json[0].name, "1") == 0);
    REQUIRE( strcmp(test->ws->json[1].name, "2") == 0);
    REQUIRE( strcmp(test->ws->json[3].name, "4") == 0);

    TEARDOWN((void **)&test, 0);
}

TEST_CASE( "Unittest - Make sure we clear a workspace correctly",  
  	   "[Workspace]" )
{
    struct workspaceTest *test = NULL;
    INIT_MOCK((void **)&test);

    test->ws->fd = 10;
    test->ws->json[0].focused = true;
    test->ws->json[1].focused = false;
    test->ws->json[3].focused = false;
    test->ws->json[0].active = true;
    test->ws->json[1].active = true;
    test->ws->json[3].active = true;
    test->ws->json[0].num = 1;
    test->ws->json[1].num = 2;
    test->ws->json[3].num = 4;
    strcpy(test->ws->json[0].name, "1");
    strcpy(test->ws->json[1].name, "2");
    strcpy(test->ws->json[3].name, "4");


    test->ws->internal->json = 
	(char *)malloc(sizeof(EMPTYWORKSPACE) * sizeof(char));

    strcpy(test->ws->internal->json, EMPTYWORKSPACE);
    test->ws->internal->lenjson = sizeof(EMPTYWORKSPACE);

    workspace_parseEvent(&test->task, test->ws);

    free(test->ws->internal->json);
    REQUIRE( test->task.exitStatus == 0 );
    REQUIRE( test->task.nextTask == NULL );
    REQUIRE( test->ws->json[0].focused == true);
    REQUIRE( test->ws->json[1].focused == false);
    REQUIRE( test->ws->json[3].focused == false);
    REQUIRE( test->ws->json[0].active == true);
    REQUIRE( test->ws->json[1].active == true);
    REQUIRE( test->ws->json[3].active == false);
    REQUIRE( test->ws->json[0].num == 1);
    REQUIRE( test->ws->json[1].num == 2);
    REQUIRE( test->ws->json[3].num == 3);
    REQUIRE( strcmp(test->ws->json[0].name, "1") == 0);
    REQUIRE( strcmp(test->ws->json[1].name, "2") == 0);
    REQUIRE( strcmp(test->ws->json[3].name, "4") == 0);

    TEARDOWN((void **)&test, 0);
}

TEST_CASE( "Unittest - Make sure we parse a change in the current workspace correctly",  
  	   "[Workspace]" )
{
    struct workspaceTest *test = NULL;
    INIT_MOCK((void **)&test);

    test->ws->fd = 10;
    test->ws->json[0].focused = true;
    test->ws->json[1].focused = false;
    test->ws->json[0].num = 1;
    test->ws->json[1].num = 2;
    strcpy(test->ws->json[0].name, "1");
    strcpy(test->ws->json[1].name, "2");


    test->ws->internal->json = 
	(char *)malloc(sizeof(CHANGEFOCUS) * sizeof(char));

    strcpy(test->ws->internal->json, CHANGEFOCUS);
    test->ws->internal->lenjson = sizeof(CHANGEFOCUS);

    workspace_parseEvent(&test->task, test->ws);

    free(test->ws->internal->json);
    REQUIRE( test->task.exitStatus == 0 );
    REQUIRE( test->task.nextTask == NULL );
    REQUIRE( test->ws->json[0].focused == false);
    REQUIRE( test->ws->json[1].focused == true);
    REQUIRE( test->ws->json[0].active == true);
    REQUIRE( test->ws->json[1].active == true);
    REQUIRE( test->ws->json[0].num == 1);
    REQUIRE( test->ws->json[1].num == 2);
    REQUIRE( strcmp(test->ws->json[0].name, "1") == 0);
    REQUIRE( strcmp(test->ws->json[1].name, "2") == 0);

    TEARDOWN((void **)&test, 0);
}
