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

/******************************************************************************
 * Declarations
 *****************************************************************************/
struct workspace;
typedef void (*_workspace_entryPoint_)(struct workspace *);
typedef struct workspace_internal__ *workspace_internal;

/******************************************************************************
 * Structs
 *****************************************************************************/
struct parsedJsonInformation 
{
    bool focused;
    char name[128];
    int num;
};

struct workspace 
{
    int fd;
    char i3Path[100];
    uint8_t numberws;
    struct parsedJsonInformation json[10];
    _workspace_entryPoint_ event;
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
#include "jsmn.h"
#include <unistd.h>

/******************************************************************************
 * enum internal to workspace
 *****************************************************************************/
enum I3_TYPE {
    COMMAND = 0,
    SUBSCRIBE = 1
};

struct workspace_internal__ {
    char json_i3[1024*4];
    size_t lenjson_i3;
};

/******************************************************************************
 * Local function declarations
 *****************************************************************************/
#if WORKSPACE_PRIVATE
#define workspacePrivate
#else 
#define workspacePrivate static
#endif
workspacePrivate void workspace_setup(struct workspace *);
workspacePrivate void workspace_entryPoint(struct workspace *);
workspacePrivate void workspace_setupSocket(
	struct taskRunner *, 
	void *);
workspacePrivate void workspace_subscribeWorkspace(
	struct taskRunner *task,
	void *_ws_);
workspacePrivate void event_startWorkspace(
	struct taskRunner *task,
	void *_ws_);
workspacePrivate void workspace_eventWorkspace(
	struct taskRunner *, 
	void *);



/* TODO the functions below */
workspacePrivate void formatMessage(enum I3_TYPE type, unsigned char *packet);
workspacePrivate void parseChangefocus(jsmn_parser parser, jsmntok_t *token, 
	int numberTokens, struct workspace *ws);
workspacePrivate void parseChangeinit(jsmn_parser parser, jsmntok_t *token, 
	int numberTokens, struct workspace *ws);
workspacePrivate int jsonParseMessageCommand(struct workspace *ws);
workspacePrivate int jsonParseMessageEvent(struct workspace *ws);
workspacePrivate int jsoneq(const char *json, jsmntok_t *tok, const char *s);

#endif /* __WORKSPACE__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _WORKSPACE_H_ */
