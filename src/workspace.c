/******************************************************************************
 * PreProcessor directive
 *****************************************************************************/
#define _XOPEN_SOURCE
#define __WORKSPACE__
#include "workspace.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>

#define NBR_JSMN_TOKENS 1024
#include "task-runner.h"

/******************************************************************************
 * inlined function declarations
 *****************************************************************************/

static inline void formatMessage(enum I3_TYPE, unsigned char *);
static inline int jsoneq(const char *, jsmntok_t *, const char *);
static inline void parseChangefocus(
	jsmn_parser, 
	jsmntok_t *, 
	int, 
	struct workspace *);
static inline void parseChangeinit(
	jsmn_parser, 
	jsmntok_t *, 
	int, 
	struct workspace *);
static inline void parseChangeempty(
	jsmn_parser, 
	jsmntok_t *, 
	int, 
	struct workspace *);

/******************************************************************************
 * exported functions declaration
 *****************************************************************************/
/*
 * Setup function used to access
 * the functionallity in this module
 */
struct workspace *workspace_init(char *i3path)
{

    struct workspace *ws = calloc(1, sizeof(struct workspace)); 

    if(ws == NULL){
	return NULL;
    }

    struct workspace_internal__
	*internal = calloc(1, sizeof(struct workspace_internal__));

    ws->internal = internal;

    if(internal == NULL){
	free(ws);
	return NULL;
    }

    ws->setup = workspace_setup;
    ws->event = workspace_entryPoint;
    strcpy(ws->i3path, i3path);

    return ws;
}

/*
 * destory function cleaing up all the memory 
 * used by this module
 */
int workspace_destroy(struct workspace *ws)
{
    free(ws->internal);
    ws->internal = NULL;

    free(ws);
    ws = NULL;
    return 0;
}

/******************************************************************************
 * Workspace entry functions
 *****************************************************************************/
private_ void workspace_setup(struct workspace *ws)
{
    struct taskRunner task = {0};
    task.nextTask = workspace_setupSocket;
    task.arg = ws;
    taskRunner_runTask(task);
}

private_ void workspace_entryPoint(struct workspace *ws)
{
    struct taskRunner task = {0};
    task.nextTask = workspace_eventWorkspace;
    task.arg = ws;
    taskRunner_runTask(task);
}

/******************************************************************************
 * Local function declarations
 *****************************************************************************/
/*
 * Sets up a file desciptor to be used for communication with
 * i3 events and commands.
 */
private_ void workspace_setupSocket(
	struct taskRunner *task,
	void *_ws_)
{
    struct workspace *ws = _ws_;
     ws->fd = SOCKET(PF_LOCAL, SOCK_STREAM, 0);

    if(ws->fd < 0){
	task->exitStatus = -1;
	task->nextTask   = NULL;
	return;
    }

    struct sockaddr_un addr = {0}; 
    addr.sun_family = AF_LOCAL;
    strncpy(addr.sun_path, ws->i3path, strlen(ws->i3path));

    if(CONNECT(ws->fd, (const struct sockaddr *)&addr, sizeof(addr)) < 0){
	task->exitStatus = -1;
	task->nextTask   = NULL;
        return;
    }

    task->exitStatus = 0;
    task->nextTask   = workspace_subscribeWorkspace;
}

/*
 * Tells i3 which
 * events we are interested in 
 */
private_ void workspace_subscribeWorkspace(
	struct taskRunner *task,
	void *_ws_)
{

    struct workspace *ws = _ws_;
    unsigned char subscribe[14];
    formatMessage(SUBSCRIBE, subscribe);
    if(WRITE(ws->fd, subscribe, sizeof(subscribe)) == 0){
	task->nextTask = NULL;
	task->exitStatus = -1;
	return;
    }

    char event[16] = "[ \"workspace\" ]";
    if(WRITE(ws->fd, event, sizeof(event)) == 0){
	task->nextTask = NULL;
	task->exitStatus = -1;
	return;
    }

    unsigned char reply[20] = {0};
    int i = READ(ws->fd, (void *)&reply[0], 14);
    if(i < -1){
	perror("write");
	task->nextTask = NULL;
	task->exitStatus = -1;
	return;
    }

    i = READ(ws->fd, (void *)&reply[0], reply[6]);
   
    if(i < -1 || strcmp((char *)reply, "{\"success\":true}") != 0){
	task->nextTask = NULL;
	task->exitStatus = -1;
	return;
    }
   
    task->nextTask = workspace_startWorkspace;
    task->exitStatus = 0;
}

/*
 * Make sure we have the current
 * state of i3 
 */
private_ void workspace_startWorkspace(
	struct taskRunner *task,
	void *_ws_)
{
    struct workspace *ws = _ws_;
    unsigned char request[14] = {0};
    
    formatMessage(COMMAND, request);
    if(WRITE(ws->fd, request, sizeof(request)) == 0){
	task->nextTask   = NULL;
	task->exitStatus = -1;
	return;
    }

    unsigned char reply[14] = {0};
    int i = READ(ws->fd, &reply[0], sizeof(reply));

    if(i < 0){
	task->nextTask   = NULL;
	task->exitStatus = -1;
	return;
    }
    
    uint32_t len = 0;
    len += (uint32_t)reply[6] << (8*0);
    len += (uint32_t)reply[7] << (8*1);
    
    ws->internal->lenjson = len + 1;
    ws->internal->json = malloc(sizeof(char) * len + 1);

    // TODO: DO IN WHILE LOOP
    ws->internal->lenjson = READ(ws->fd, ws->internal->json, len);

    task->nextTask   = workspace_parseInitWorkspace;
    task->exitStatus = 0;
}

/*
 * Parses inital state of i3 workspace
 */
private_ void workspace_parseInitWorkspace(
	struct taskRunner *task,
	void *_ws_)
{
    struct workspace *ws = _ws_;
    jsmn_parser parser;
    jsmntok_t token[NBR_JSMN_TOKENS];

    jsmn_init(&parser);

    int ret = jsmn_parse(&parser, ws->internal->json, 
	    ws->internal->lenjson, token, NBR_JSMN_TOKENS);

    if(ret < 1){
	task->exitStatus = -1;
	task->nextTask   = NULL;
       return;
    }

    int n = -1;
    for(int i = 0; i < ret; i++){
	if(token[i].type == JSMN_OBJECT && 
	   jsoneq(ws->internal->json, &token[i - 1], "rect") != 0){
	    n++;
	}
	if(jsoneq(ws->internal->json, &token[i], "focused") == 0){
	    char boolean[6] = {0};
	    strncpy(boolean, ws->internal->json + token[i + 1].start, 
		    token[i + 1].end - token[i + 1].start);
	    if(strcmp(boolean, "true") == 0){
		ws->json[n].focused = true;
	    } else {
		ws->json[n].focused = false;
	    }
	    ws->json[n].active = true;
	    i++;
	} else if(jsoneq(ws->internal->json, &token[i], "name") == 0){
	    strncpy(ws->json[n].name, 
		    ws->internal->json + token[i + 1].start, 
		    token[i + 1].end - token[i + 1].start);
	    i++;
	} else if(jsoneq(ws->internal->json, &token[i], "num") == 0){
	    char value[10] = {0};
	    strncpy(value, ws->internal->json + token[i + 1].start, 
		    token[i + 1].end - token[i + 1].start);
	    ws->json[n].num = strtol(value, NULL, 10);
	    ws->json[n].active = true;
	    i++;
	}
    }

    free(ws->internal->json);
    ws->internal->json = NULL;
    ws->internal->lenjson = 0;
    task->exitStatus = 0;
    task->nextTask   = NULL;
}

/*
 * Entry point for workspace when the workspace file descriptor
 * is ready to read from.
 */
private_ void workspace_eventWorkspace(
	struct taskRunner *task, 
	void * _ws_)
{
    struct workspace *ws = _ws_;
    unsigned char event[14] = {0};

    int i = READ(ws->fd, &event[0], sizeof(event));
    
    if(i < 0){
	task->nextTask   = NULL;
	task->exitStatus = -1;
	return;
    }

    uint32_t len = 0;
    len += (uint32_t)event[6] << (8*0);
    len += (uint32_t)event[7] << (8*1);

    ws->internal->json = malloc(sizeof(char)*len);

    ws->internal->lenjson = READ(ws->fd, ws->internal->json, len);

    task->exitStatus = 0;
    task->nextTask   = workspace_parseEvent;
}


/*
 * Parse the current recived event from
 * i3 supports init and focus
 */
private_ void workspace_parseEvent(
	struct taskRunner *task,
	void *_ws_)
{
    struct workspace *ws = _ws_;
    jsmn_parser parser;
    jsmntok_t token[NBR_JSMN_TOKENS];

    jsmn_init(&parser);

    int numberTokens = jsmn_parse(&parser, ws->internal->json, 
	    ws->internal->lenjson, token, NBR_JSMN_TOKENS);
    

    if(numberTokens < 1){
	task->exitStatus = -1;
	task->nextTask   = NULL;
	return;
    }

    enum I3_EVENT event = UNKNOWN;    
    if(jsoneq(ws->internal->json, &token [1], "change") == 0 &&
       jsoneq(ws->internal->json, &token [2], "focus") == 0){
	event = FOCUS;
    }

    if(jsoneq(ws->internal->json, &token [1], "change") == 0 &&
       jsoneq(ws->internal->json, &token [2], "init") == 0){
	event = INIT;
    }
    
    if(jsoneq(ws->internal->json, &token [1], "change") == 0 &&
       jsoneq(ws->internal->json, &token [2], "empty") == 0){
	event = EMPTY;
    }

    switch(event){
	case INIT:
	    parseChangeinit(parser, token, numberTokens, ws);
	    break;
	case FOCUS:
	    parseChangefocus(parser, token, numberTokens, ws);
	    break;
	case EMPTY:
	    parseChangeempty(parser, token, numberTokens, ws);
	default:
	    break;
    }

    free(ws->internal->json);
    ws->internal->json = NULL;
    task->exitStatus = 0;
    task->nextTask   = NULL;

}

/******************************************************************************
 * Inlined small functions (should only be used once)
 *****************************************************************************/
/*
 * Parses the json when reciving 
 * a change in focus and fills in the information
 * in the ws
 */
static inline void parseChangefocus(
	jsmn_parser parser, 
	jsmntok_t *token, 
	int numberTokens, 
	struct workspace *ws)
{
    bool current = true;
    for(int i = 1; i < numberTokens; i++){
	 if(token[i].type == JSMN_OBJECT && 
	    jsoneq(ws->internal->json, &token[i - 1], "old") == 0){
	     current = false;
	 }
	if(jsoneq(ws->internal->json, &token[i], "num") == 0){
	    char value[10] = {0};
	    strncpy(value, ws->internal->json + token[i + 1].start, 
		    token[i + 1].end - token[i + 1].start);
	    int num = strtol(value, NULL, 10) - 1;
	    ws->json[num].focused = current ? true: false;
	    ws->json[num].active = true;
	    i++;
	} 
    }
}

/*
 * Parses the json when reciving 
 * a init that a new workspace has 
 * been created fills in the information
 * in the ws
 */
static inline void parseChangeinit(
	jsmn_parser parser, 
	jsmntok_t *token, 
	int numberTokens, 
	struct workspace *ws)
{
    int num = 0;
    char name[128] = {0};

    for(int i = 0; i < NUMBER_WORKSPACES; i++){
	ws->json[i].focused = false;
    }

    for(int i = 1; i < numberTokens; i++){
	if(jsoneq(ws->internal->json, &token[i], "num") == 0){
	    char value[10] = {0};
	    strncpy(value, ws->internal->json + token[i + 1].start, 
		    token[i + 1].end - token[i + 1].start);
	    num = strtol(value, NULL, 10) - 1;
	    ws->json[num].focused = true;
	    ws->json[num].num = num;
	    ws->json[num].active = true;
	    i++;
	} else if(jsoneq(ws->internal->json, &token[i], "name") == 0){
	    strncpy(name, ws->internal->json + token[i + 1].start, 
		    token[i + 1].end - token[i + 1].start);
	    i++;
	} 
    }
    strcpy(ws->json[num].name, name);
}

/*
 * Parses the json when reciving 
 * a init that a new workspace has 
 * been created fills in the information
 * in the ws
 */
static inline void parseChangeempty(
	jsmn_parser parser, 
	jsmntok_t *token, 
	int numberTokens, 
	struct workspace *ws)
{
    int num = 0;
    char name[128] = {0};

    for(int i = 1; i < numberTokens; i++){
	if(jsoneq(ws->internal->json, &token[i], "num") == 0){
	    char value[10] = {0};
	    strncpy(value, ws->internal->json + token[i + 1].start, 
		    token[i + 1].end - token[i + 1].start);
	    num = strtol(value, NULL, 10) - 1;
	    ws->json[num].focused = false;
	    ws->json[num].num = num;
	    ws->json[num].active = false;
	    i++;
	} else if(jsoneq(ws->internal->json, &token[i], "name") == 0){
	    strncpy(name, ws->internal->json + token[i + 1].start, 
		    token[i + 1].end - token[i + 1].start);
	    i++;
	} 
    }
    strcpy(ws->json[num].name, name);
}

/*
 * Defines what to be sent to i3 
 * subscription or command
 */
static inline void formatMessage(enum I3_TYPE type, unsigned char *packet)
{
    switch(type){
	case SUBSCRIBE:
	    memcpy(&packet[0], "i3-ipc\x0F\x0\x0\x0\x2\x0\x0", 14);
	    break;
	case COMMAND:
	    memcpy(&packet[0], "i3-ipc\x0\x0\x0\x0\x1\x0\x0", 14);
	    break;
	default:
	    ; // pass
	    break;
    }

}

/*
 * Check if the current token
 * is the token we are currently looking 
 * for
 */
static inline int jsoneq(const char *json, jsmntok_t *tok, const char *s) 
{
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
	    strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
	return 0;
    }
    return -1;
}
