/******************************************************************************
 * PreProcessor directive
 *****************************************************************************/
#define _XOPEN_SOURCE
#define __WORKSPACE__

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
#define JSMN_STATIC
#include "workspace.h"
#include "task-runner.h"

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
    struct workspace_internal__
	*internal = calloc(1, sizeof(struct workspace_internal__));

    ws->internal = internal;

    if(internal == NULL){
	free(ws);
	return NULL;
    }

    ws->setup = workspace_setup;
    ws->event = workspace_entryPoint;

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
workspacePrivate void workspace_setup(struct workspace *ws)
{
    struct taskRunner task = {0};
    task.nextTask = workspace_setupSocket;
    taskRunner_runTask(task);
}

workspacePrivate void workspace_entryPoint(struct workspace *ws)
{
    struct taskRunner task = {0};
    task.nextTask = workspace_eventWorkspace;
    taskRunner_runTask(task);
}

/******************************************************************************
 * Local function declarations
 *****************************************************************************/
/*
 * Sets up a file desciptor to be used for communication with
 * i3 events and commands.
 */
workspacePrivate void workspace_setupSocket(
	struct taskRunner *task,
	void *_ws_)
{
    struct workspace *ws = (struct workspace *)_ws_;
    int i3Sock = socket(PF_LOCAL, SOCK_STREAM, 0);
    ws->fd = i3Sock;
    if(i3Sock < 0){
	task->exitStatus = -1;
	task->nextTask   = NULL;
	return;
    }

    struct sockaddr_un addr = {0}; 
    addr.sun_family = AF_LOCAL;
    strncpy(addr.sun_path, ws->i3Path, strlen(ws->i3Path));

    if(connect(ws->fd, (const struct sockaddr *)&addr, sizeof(addr)) < 0){
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
workspacePrivate void workspace_subscribeWorkspace(
	struct taskRunner *task,
	void *_ws_)
{

    struct workspace *ws = (struct workspace *)_ws_;
    unsigned char subscribe[14];
    formatMessage(SUBSCRIBE, subscribe);
    if(write(ws->fd, subscribe, sizeof(subscribe)) == 0){
	task->nextTask = NULL;
	task->exitStatus = -1;
	return;
    }

    char event[16] = "[ \"workspace\" ]";
    if(write(ws->fd, event, sizeof(event)) == 0){
	task->nextTask = NULL;
	task->exitStatus = -1;
	return;
    }

    unsigned char reply[20] = {0};
    int i = read(ws->fd, &reply[0], 14);
    if(i < -1){
	task->nextTask = NULL;
	task->exitStatus = -1;
	return;
    }

    i = read(ws->fd, &reply[0], reply[6]);
   
    /* Check that we recive success json here */ 
    if(i < -1){
	task->nextTask = NULL;
	task->exitStatus = -1;
	return;
    }
   
    task->nextTask = event_startWorkspace;
    task->exitStatus = 0;
}

/*
 * Make sure we have the current
 * state of i3 
 */
workspacePrivate void event_startWorkspace(
	struct taskRunner *task,
	void *_ws_)
{
    struct workspace *ws = (struct workspace *)_ws_;
    unsigned char request[14] = {0};
    
    formatMessage(COMMAND, request);
    if(write(ws->fd, request, sizeof(request)) == 0){
	task->nextTask   = NULL;
	task->exitStatus = -1;
	return;
    }

    unsigned char reply[14] = {0};
    int i = read(ws->fd, &reply[0], sizeof(reply));

    if(i < 0){
	task->nextTask   = NULL;
	task->exitStatus = -1;
	return;
    }
    
    uint32_t len = 0;
    len += (uint32_t)reply[6] << (8*0);
    len += (uint32_t)reply[7] << (8*1);

    ws->internal->lenjson_i3 = read(ws->fd, &(ws->internal->json_i3[0]), len);

    task->nextTask   = NULL;
    task->exitStatus = 0;
}


/*
 * Entry point for workspace when the workspace file descriptor
 * is ready to read from.
 */
workspacePrivate void workspace_eventWorkspace(
	struct taskRunner *task, 
	void * _ws_)
{
    struct workspace *ws = (struct workspace *)_ws_;
    unsigned char event[14] = {0};

    int i = read(ws->fd, &event[0], sizeof(event));
    
    if(i < 0){
	task->nextTask   = NULL;
	task->exitStatus = -1;
	return;
    }

    uint32_t len = 0;
    len += (uint32_t)event[6] << (8*0);
    len += (uint32_t)event[7] << (8*1);

    ws->internal->lenjson_i3 = read(ws->fd, (&(ws->internal->json_i3))[0], len);

    task->exitStatus = 0;
    task->nextTask   = NULL;
}


/*
 * TODO: below
 */
workspacePrivate int jsoneq(const char *json, jsmntok_t *tok, const char *s) 
{
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
	    strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
	return 0;
    }
    return -1;
}


workspacePrivate int jsonParseMessageEvent(struct workspace *ws)
{
    jsmn_parser parser;
    jsmntok_t token[NBR_JSMN_TOKENS];

    jsmn_init(&parser);

    int numberTokens = jsmn_parse(&parser, ws->internal->json_i3, 
	    ws->internal->lenjson_i3, token, 
				  NBR_JSMN_TOKENS);

    if(numberTokens < 1){
	return -1;
    }
    
    if(jsoneq(ws->internal->json_i3, &token [1], "change") == 0 &&
       jsoneq(ws->internal->json_i3, &token [2], "focus") == 0){
	parseChangefocus(parser, token, numberTokens, ws);
	return 0;
    }

    if(jsoneq(ws->internal->json_i3, &token [1], "change") == 0 &&
       jsoneq(ws->internal->json_i3, &token [2], "init") == 0){
	parseChangeinit(parser, token, numberTokens, ws);
	return 0;
    }

    return -1;
}

workspacePrivate int jsonParseMessageCommand(struct workspace *ws)
{
    jsmn_parser parser;
    jsmntok_t token[NBR_JSMN_TOKENS];

    jsmn_init(&parser);

    int ret = jsmn_parse(&parser, ws->internal->json_i3, 
	    ws->internal->lenjson_i3, token, NBR_JSMN_TOKENS);

    if(ret < 1){
       return -1;
    }

    int n = -1;
    for(int i = 0; i < ret; i++){
	if(token[i].type == JSMN_OBJECT && 
	   jsoneq(ws->internal->json_i3, &token[i - 1], "rect") != 0){
	    n++;
	}
	if(jsoneq(ws->internal->json_i3, &token[i], "focused") == 0){
	    char boolean[6] = {0};
	    strncpy(boolean, ws->internal->json_i3 + token[i + 1].start, 
		    token[i + 1].end - token[i + 1].start);
	    if(strcmp(boolean, "true") == 0){
		ws->json[n].focused = true;
	    } else {
		ws->json[n].focused = false;
	    }
	    i++;
	} else if(jsoneq(ws->internal->json_i3, &token[i], "name") == 0){
	    strncpy(ws->json[n].name, 
		    ws->internal->json_i3 + token[i + 1].start, 
		    token[i + 1].end - token[i + 1].start);
	    i++;
	} else if(jsoneq(ws->internal->json_i3, &token[i], "num") == 0){
	    char value[10] = {0};
	    strncpy(value, ws->internal->json_i3 + token[i + 1].start, 
		    token[i + 1].end - token[i + 1].start);
	    ws->json[n].num = strtol(value, NULL, 10);
	    i++;
	}
    }

    ws->numberws = n;
    return 0;
}

workspacePrivate void parseChangefocus(
	jsmn_parser parser, jsmntok_t *token, 
	int numberTokens, struct workspace *ws)
{
    bool current = true;
    for(int i = 0; i < numberTokens; i++){
	if(token[i].type == JSMN_OBJECT && 
	   jsoneq(ws->internal->json_i3, &token[i - 1], "old") == 0){
	    current = false;
	}
	if(jsoneq(ws->internal->json_i3, &token[i], "num") == 0){
	    char value[10] = {0};
	    strncpy(value, ws->internal->json_i3 + token[i + 1].start, 
		    token[i + 1].end - token[i + 1].start);
	    int num = strtol(value, NULL, 10) - 1;
	    ws->json[num].focused = current ? true: false;
	    i++;
	} 
    }
}

workspacePrivate void parseChangeinit(jsmn_parser parser, jsmntok_t *token, 
	int numberTokens, struct workspace *ws)
{
    int num = 0;
    char name[128] = {0};
    for(int i = 0; i < numberTokens; i++){
	if(jsoneq(ws->internal->json_i3, &token[i], "num") == 0){
	    char value[10] = {0};
	    strncpy(value, ws->internal->json_i3 + token[i + 1].start, 
		    token[i + 1].end - token[i + 1].start);
	    num = strtol(value, NULL, 10) - 1;
	    ws->json[num].focused = true;
	    i++;
	} else if(jsoneq(ws->internal->json_i3, &token[i], "name") == 0){
	    strncpy(name, ws->internal->json_i3 + token[i + 1].start, 
		    token[i + 1].end - token[i + 1].start);
	    i++;
	} 
    }
    strcpy(ws->json[num].name, name);
    ws->numberws = num;
}


workspacePrivate void formatMessage(enum I3_TYPE type, unsigned char *packet)
{
    switch(type){
	case SUBSCRIBE:
	    memcpy(&packet[0], "i3-ipc\x15\x0\x0\x0\x2\x0\x0", 14);
	    break;
	case COMMAND:
	    memcpy(&packet[0], "i3-ipc\x0\x0\x0\x0\x1\x0\x0", 16);
	    break;
	default:
	    ; // pass
	    break;
    }

}
