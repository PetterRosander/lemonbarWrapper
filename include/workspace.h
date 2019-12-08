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

/******************************************************************************
 * Declarations
 *****************************************************************************/
struct workspace;
typedef int (*eventWorkspace)(struct workspace *);
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
    uint8_t numberws;
    struct parsedJsonInformation json[10];
    eventWorkspace event;
    workspace_internal internal;
};

/******************************************************************************
 * Exported function
 *****************************************************************************/
struct workspace *workspace_init(char *i3path);
int workspace_destroy(struct workspace *ws);

/******************************************************************************
 * Internal struct for workspace
 *****************************************************************************/
#ifdef __WORKSPACE__
#define NBR_JSMN_TOKENS 1024
#define JSMN_STATIC
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
#ifndef workspacePrivate
#define workspacePrivate static
#endif
workspacePrivate void formatMessage(enum I3_TYPE type, unsigned char *packet);
workspacePrivate void parseChangefocus(jsmn_parser parser, jsmntok_t *token, 
	int numberTokens, struct workspace *ws);
workspacePrivate void parseChangeinit(jsmn_parser parser, jsmntok_t *token, 
	int numberTokens, struct workspace *ws);

workspacePrivate int setupSocket(struct workspace *ws, char *i3path);
workspacePrivate int subscribeWorkspace(struct workspace *ws);
workspacePrivate int startWorkSpace(struct workspace *ws);

workspacePrivate int eventWorkspace__(struct workspace *ws);

#endif /* __WORKSPACE__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _WORKSPACE_H_ */
