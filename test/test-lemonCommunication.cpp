/******************************************************************************
 * PreProcessor directive
 *****************************************************************************/
#define __LEMON__
#include "lemonCommunication.h"
#include "workspace.h"

#include <string.h>

#include "catch2/catch.hpp"
#include "mock-symbol.hpp"
#include "mock-approvals.hpp"

struct lemonTest {
    struct taskRunner task;
    struct lemonbar *lm;
};

static void setup(void **state)
{
    struct lemonTest *test = 
	(struct lemonTest *)calloc(1, sizeof(struct lemonTest));
    test->lm = lemon_init();
    *state = test;
}

static void teardown(void **state)
{
    struct lemonTest *test = (struct lemonTest *)*state;
    lemon_destroy(test->lm);
    free(test);
}


TEST_CASE("Accpetancetest - Make sure we write the same thing to lemonbar"
	  "given an input", "[lemon]")
{
    struct lemonTest *test = NULL;
    INIT_MOCK((void **)&test);

    struct workspace ws = {0};
    /* workspace 1: */
    ws.json[0].active = true;
    ws.json[0].focused = false;
    ws.json[0].num = 1;
    strcpy(ws.json[0].name, "dev");

    /* workspace 2: */
    ws.json[1].active = true;
    ws.json[1].focused = false;
    ws.json[1].num = 2;
    strcpy(ws.json[1].name, "web");

    /* workspace 3: */
    ws.json[5].active = true;
    ws.json[5].focused = true;
    ws.json[5].num = 6;
    strcpy(ws.json[5].name, "music");
    
    /* workspace 4: */
    ws.json[8].active = true;
    ws.json[8].focused = false;
    ws.json[8].num = 9;
    strcpy(ws.json[8].name, "chill");


    test->lm->action = WORKSPACE;
    test->lm->ws = &ws;

    lemon_formatWorkspace(&test->task, test->lm);

    approval approval("test/approval-files/lemon-workspace-format.approved");
    REQUIRE( approval.testEquality(test->lm->internal->lemonFormat) == true );

    TEARDOWN((void **)&test, 0);
}
