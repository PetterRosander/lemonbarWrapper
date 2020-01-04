#ifndef _WORKSPACE_H_
#define _WORKSPACE_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
/******************************************************************************
 * PreProcessor directives
 *****************************************************************************/
#include <sys/types.h>
#include <stdbool.h>
#include <stdint.h>
#include "task-runner.h"
#define NUMBER_WORKSPACES 10

/******************************************************************************
 * Declarations
 *****************************************************************************/
struct workspace;
typedef void (*_workspace_entryPoint_)(struct taskRunner *, struct workspace *);
typedef struct workspace_internal__ *workspace_internal;

/******************************************************************************
 * Structs
 *****************************************************************************/
struct parsedJsonInformation 
{
    bool focused;
    bool active;
    char name[128];
    int num;
};

struct workspace 
{
    int fd;
    char i3path[100];
    struct parsedJsonInformation json[NUMBER_WORKSPACES];

    _workspace_entryPoint_ event;
    _workspace_entryPoint_ reconnect;
    _workspace_entryPoint_ setup;
    workspace_internal internal;
};

/******************************************************************************
 * Exported function
 *****************************************************************************/
struct workspace *workspace_init(char *);
int workspace_destroy(struct workspace *);

/******************************************************************************
 * Internal struct for workspace
 *****************************************************************************/
#ifdef __WORKSPACE__
#define JSMN_HEADER
#include "jsmn.h"
#include <unistd.h>

/******************************************************************************
 * enum internal to workspace
 *****************************************************************************/
enum I3_TYPE {
    COMMAND = 0,
    SUBSCRIBE = 1
};

enum I3_EVENT {
    UNKNOWN = 0,
    INIT    = 1,
    FOCUS   = 2,
    EMPTY   = 3
};

struct workspace_internal__ {
    bool reconnect;
    char *json;
    size_t lenjson;
};

/******************************************************************************
 * Local function declarations
 *****************************************************************************/
#if UNIT_TEST
#define private_
#else 
#define private_ static
#endif
#include "system-calls.h"

private_ void workspace_setup(
	struct taskRunner *,
	struct workspace *);

private_ void workspace_entryPoint(
	struct taskRunner *, 
	struct workspace *);
private_ void workspace_reconnect(
	struct taskRunner *,
	struct workspace *);

private_ void workspace_waitSocket(
	struct taskRunner *, 
	void *);
private_ void workspace_setupSocket(
	struct taskRunner *, 
	void *);
private_ void workspace_subscribeWorkspace(
	struct taskRunner *,
	void *);
private_ void workspace_startWorkspace(
	struct taskRunner *,
	void *);
private_ void workspace_eventWorkspace(
	struct taskRunner *, 
	void *);
private_ void workspace_parseInitWorkspace(
	struct taskRunner *,
	void *);
private_ void workspace_parseEvent(
	struct taskRunner *,
	void *);

#endif /* __WORKSPACE__ */
#undef __WORKSPACE__

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _WORKSPACE_H_ */
