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
#include "configuration-manager.h"
#include "task-runner.h"

/******************************************************************************
 * Declarations
 *****************************************************************************/
struct lemonbar;
typedef void (*_lemonbar_entryPoint_)(struct taskRunner *, struct lemonbar *);
typedef struct lemon_internal__ *lemon_internal;

/******************************************************************************
 * Structs
 *****************************************************************************/
struct lemonConfig {
    char const *lemonArgs;
};

struct lemonbar
{
    int pipeRead;
    struct workspace *ws;
    struct plugins *pl;

    struct lemonConfig lmcfg;

    _lemonbar_entryPoint_ setup;
    _lemonbar_entryPoint_ addFd;
    _lemonbar_entryPoint_ render;
    _lemonbar_entryPoint_ reconfigure;
    _lemonbar_entryPoint_ action;
    lemon_internal internal;
};

/******************************************************************************
 * Exported function
 *****************************************************************************/
struct lemonbar *lemon_init(struct configuration *cfg);
int lemon_destroy(struct lemonbar *);

/******************************************************************************
 * Internal struct for workspace
 *****************************************************************************/
#ifdef __LEMON__
#include "task-runner.h"
#include "workspace.h"
#include "plugins.h"

/******************************************************************************
 * enum internal to workspace
 *****************************************************************************/
struct lemon_internal__ {
    int pipeWrite;
    pid_t pid;
    size_t lenFormat;
    char lemonFormat[1024];
};

/******************************************************************************
 * Local function declarations
 *****************************************************************************/
#if UNIT_TEST
#define private_
#else 
#define private_ static
#endif

private_ void lemon_setup(
	struct taskRunner *,
	struct lemonbar *);
private_ void lemon_addFd(
	struct taskRunner *,
	struct lemonbar *);
private_ void lemon_reRender(
	struct taskRunner *,
	struct lemonbar *);
private_ void lemon_reconfigure(
	struct taskRunner *,
	struct lemonbar *);
private_ void lemon_action(
	struct taskRunner *,
	struct lemonbar *);

private_ void lemon_pluginAction(
	struct taskRunner *,
	void *);
private_ void lemon_handleError(
	struct taskRunner *,
	void *);
private_ void lemon_lockOrShutdown(
	struct taskRunner *,
	void * );
private_ void lemon_setupCommunication(
	struct taskRunner *,
	void *);
private_ void lemon_teardownCommunication(
	struct taskRunner *,
	void *);
private_ void lemon_sendLemonbar(
	struct taskRunner *,
	void * );
private_ void lemon_formatNormal(
	struct taskRunner *,
	void *);

#endif /*  __LEMON__ */
#undef __LEMON__

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _LEMONCOMMUNICATION_H_ */
