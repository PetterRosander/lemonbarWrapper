#ifndef _LEMONCOMMUNICATION_H_
#define _LEMONCOMMUNICATION_H_

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************
 * PreProcessor directives
 *****************************************************************************/
#include <stdio.h>
#include <sys/types.h>

/******************************************************************************
 * Declarations
 *****************************************************************************/
struct lemonbar;
typedef void (*_lemonbar_entryPoint_)(struct lemonbar *);
typedef struct lemon_internal__ *lemon_internal;

/******************************************************************************
 * Structs
 *****************************************************************************/
enum lemonAction {
    WORKSPACE = 0
};

struct lemonbar
{
    int stdout;
    struct workspace *ws;
    enum lemonAction action;
    _lemonbar_entryPoint_ setup;
    _lemonbar_entryPoint_ com;
    lemon_internal internal;
};

/******************************************************************************
 * Exported function
 *****************************************************************************/
struct lemonbar *lemon_init(void);
int lemon_destroy(struct lemonbar *lm);

/******************************************************************************
 * Internal struct for workspace
 *****************************************************************************/
#ifdef __LEMON__
#undef __LEMON__
#include "task-runner.h"
#include "workspace.h"

/******************************************************************************
 * enum internal to workspace
 *****************************************************************************/
struct lemon_internal__ {
    int stdin;
    pid_t pid;
    size_t lenFormat;
    char lemonFormat[1024];
};

#if UNIT_TEST
#define private_
#else 
#define private_ static
#endif

private_ void lemon_setup(struct lemonbar *);
private_ void lemon_com(struct lemonbar *lm);
private_ void lemon_setupCommunication(
	struct taskRunner *,
	void *);
private_ void lemon_action(
	struct taskRunner *,
	void *);
private_ void lemon_formatWorkspace(
	struct taskRunner *task,
	void *);

#endif /*  __LEMON__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _LEMONCOMMUNICATION_H_ */
