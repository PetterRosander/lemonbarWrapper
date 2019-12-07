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
int workspace_init(struct workspace *ws, char *i3path);

/******************************************************************************
 * Internal struct for workspace
 *****************************************************************************/
#ifdef __WORKSPACE__
#include <unistd.h>

struct workspace_internal__ {
    char json_i3[1024*4];
    size_t lenjson_i3;
};

#endif /* __WORKSPACE__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _WORKSPACE_H_ */
