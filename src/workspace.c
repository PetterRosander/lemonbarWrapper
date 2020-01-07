/******************************************************************************
 * PreProcessor directive
 *****************************************************************************/
#define _XOPEN_SOURCE
#define __WORKSPACE__
#include "workspace.h"

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/inotify.h>
#include <errno.h>

#define NBR_JSMN_TOKENS 1024
#include "task-runner.h"
#include "configuration-manager.h"

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
struct workspace *workspace_init(char *i3path, struct configuration *cfg)
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
    
    char *value = NULL;
    hashmap_get(cfg->mcfg.configMap, "WORKSPACE_FOCUSED_COLOR", (void**)(&value));
    ws->focusedColor = value;
    hashmap_get(cfg->mcfg.configMap, "WORKSPACE_UNFOCUSED_COLOR", (void**)(&value));
    ws->unfocusedColor = value;

    ws->setup = workspace_setup;
    ws->reconnect = workspace_reconnect;
    ws->event = workspace_entryPoint;
    ws->addFd = workspace_addFd;
    strcpy(ws->i3path, i3path);

    return ws;
}

/*
 * destory function cleaing up all the memory 
 * used by this module
 */
int workspace_destroy(struct workspace *ws)
{
    if(ws->internal->json != NULL){
	free(ws->internal->json);
	ws->internal->json = NULL;
    }
    free(ws->internal);
    ws->internal = NULL;

    free(ws);
    ws = NULL;
    return 0;
}

/******************************************************************************
 * Workspace entry functions
 *****************************************************************************/
private_ void workspace_setup(
	struct taskRunner *task,
	struct workspace *ws)
{
    task->nextTask[0] = workspace_waitSocket;
    task->nextTask[1] = workspace_setupSocket;
    task->nextTask[2] = workspace_subscribeWorkspace;
    task->nextTask[3] = workspace_startWorkspace;
    task->nextTask[4] = workspace_parseInitWorkspace;
    task->nextTask[5] = workspace_formatWorkspace;
    task->nbrTasks = 6;
    task->arg = ws;
    taskRunner_runTask(task);
}

private_ void workspace_addFd(
	struct taskRunner *task,
	struct workspace *ws)
{
    /*
     * If we are disconnected try to connect
     * and add to fd
     */
    if(ws->internal->reconnect){
	workspace_reconnect(task, ws);
    }

    if(ws->fd >= 0){
	task->fds[task->nfds].fd = ws->fd;
	task->fds[task->nfds].events = POLLIN | POLLHUP;
	task->nfds++;
    } else {
	// Failed to reconnect shorten poll time
	task->poll_t = 200;
    }
}

private_ void workspace_reconnect(
	struct taskRunner *task, 
	struct workspace *ws)
{
    close(ws->fd);
    ws->fd = -1;
    ws->internal->reconnect = true;
    task->nextTask[0] = workspace_waitSocket;
    task->nextTask[1] = workspace_setupSocket;
    task->nextTask[2] = workspace_subscribeWorkspace;
    task->nbrTasks = 3;
    task->arg = ws;
    taskRunner_runTask(task);
}

private_ void workspace_entryPoint(
	struct taskRunner *task, 
	struct workspace *ws)
{
    task->nextTask[0] = workspace_eventWorkspace;
    task->nextTask[1] = workspace_parseEvent;
    task->nextTask[2] = workspace_formatWorkspace;
    task->nbrTasks = 3;
    task->arg = ws;
    taskRunner_runTask(task);
}

/******************************************************************************
 * Local function declarations
 *****************************************************************************/
/*
 * Sets up a file desciptor to be used for communication with
 * i3 events and commands.
 */
private_ void workspace_waitSocket(
	struct taskRunner *task,
	void *_ws_)
{
    struct workspace *ws = _ws_;

    struct stat buf;
    if(stat(ws->i3path, &buf) != 0){
	lemonLog(DEBUG, "%s does no exist yet - wait", ws->i3path);
	task->exitStatus = DO_NOTHING;
	return;
    }

    task->exitStatus = FINE;
}

private_ void workspace_setupSocket(
	struct taskRunner *task,
	void *_ws_)
{
    struct workspace *ws = _ws_;
    ws->fd = SOCKET(PF_LOCAL, SOCK_STREAM, 0);

    if(ws->fd < 0){
	lemonLog(ERROR, "Failed to create socket %s", strerror(errno));
	task->exitStatus = DO_NOTHING;
	return;
    }

    struct sockaddr_un addr = {0}; 
    addr.sun_family = AF_LOCAL;
    strncpy(addr.sun_path, ws->i3path, strlen(ws->i3path));

    if(CONNECT(ws->fd, (const struct sockaddr *)&addr, sizeof(addr)) < 0){
	lemonLog(ERROR, "Failed to connect with i3 socket %s", 
		strerror(errno));
	task->cleanTask = workspace_handleError;
	task->exitStatus = NON_FATAL;
	return;
    }

    task->exitStatus = FINE;
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
    if(WRITE(ws->fd, subscribe, sizeof(subscribe)) < 0){
	lemonLog(ERROR, "Failed write %s", strerror(errno));
	task->cleanTask = workspace_resetConnection;
	task->exitStatus = CRITICAL;
	return;
    }

    char event[16] = "[ \"workspace\" ]";
    if(WRITE(ws->fd, event, sizeof(event)) < 0){
	lemonLog(ERROR, "Failed write %s", strerror(errno));
	task->cleanTask = workspace_resetConnection;
	task->exitStatus = CRITICAL;
	return;
    }

    unsigned char reply[20] = {0};
    int i = READ(ws->fd, (void *)&reply[0], 14);
    if(i < -1){
	lemonLog(ERROR, "Failed read %s", strerror(errno));
	task->cleanTask = workspace_resetConnection;
	task->exitStatus = CRITICAL;
	return;
    }

    i = READ(ws->fd, (void *)&reply[0], reply[6]);

    if(i < -1 || strcmp((char *)reply, "{\"success\":true}") != 0){
	lemonLog(ERROR, "Failed to subscribe to i3 worskapce %s", 
		strerror(errno));
	task->cleanTask = workspace_resetConnection;
	task->exitStatus = CRITICAL;
	return;
    }

    if(ws->internal->reconnect){
	lemonLog(DEBUG, "Reconnected with i3 and subscribed succesfully");
	ws->internal->reconnect = false;
    } else {
	lemonLog(DEBUG, "Connected with i3 and subscribed succesfully");
    }
    task->exitStatus = FINE;
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
    if(WRITE(ws->fd, request, sizeof(request)) <= 0){
	lemonLog(ERROR, "Failed to write %s", strerror(errno));
	task->exitStatus = -1;
	return;
    }

    unsigned char reply[14] = {0};
    int i = READ(ws->fd, &reply[0], sizeof(reply));

    if(i < 0){
	lemonLog(ERROR, "Failed read %s", strerror(errno));
	task->cleanTask = workspace_resetConnection;
	task->exitStatus = CRITICAL;
	return;
    }

    uint32_t len = 0;
    len += (uint32_t)reply[6] << (8*0);
    len += (uint32_t)reply[7] << (8*1);

    ws->internal->lenjson = len + 1;
    ws->internal->json = malloc(sizeof(char) * len + 1);
    if(!ws->internal->json){
	lemonLog(ERROR, "Failed malloc out of memory?!");
	task->exitStatus = FATAL;
	return;
    }

    // TODO: DO IN WHILE LOOP
    ws->internal->lenjson = READ(ws->fd, ws->internal->json, len);
    if(ws->internal->lenjson < 0){
	lemonLog(ERROR, "Failed read %s", strerror(errno));
	task->cleanTask = workspace_resetConnection;
	task->exitStatus = CRITICAL;
	return;
    }

    task->exitStatus = FINE;
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

    int numberTokens = jsmn_parse(&parser, ws->internal->json, 
	    ws->internal->lenjson, token, NBR_JSMN_TOKENS);

    if(numberTokens < 1){
	char *JSMN_ERR[3] = {
	    "JSMN_ERROR_NOMEM", 
	    "JSMN_ERROR_INVAL", 
	    "JSMN_ERROR_PART"
	};
	lemonLog(ERROR, "jsmn faileld to parse json %s", 
		JSMN_ERR[numberTokens]);
	task->cleanTask = workspace_handleError;
	task->exitStatus = NON_FATAL;
	return;
    }

    int n = -1;
    for(int i = 0; i < numberTokens; i++){
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
    task->exitStatus = FINE;
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
	lemonLog(ERROR, "Failed to write %s", strerror(errno));
	task->exitStatus = CRITICAL;
	task->cleanTask = workspace_resetConnection;
	return;
    }

    uint32_t len = 0;
    len += (uint32_t)event[6] << (8*0);
    len += (uint32_t)event[7] << (8*1);

    ws->internal->json = malloc(sizeof(char)*len);
    if(!ws->internal->json){
	lemonLog(ERROR, "Failed malloc: %s");
	task->exitStatus = FATAL;
	return;
    }

    ws->internal->lenjson = READ(ws->fd, ws->internal->json, len);

    if(ws->internal->lenjson < 0){
	lemonLog(ERROR, "Read failed %s", strerror(errno));
	task->cleanTask = workspace_resetConnection;
	task->exitStatus = CRITICAL;
	return;
    }

    task->exitStatus = FINE;
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
	char *JSMN_ERR[3] = {
	    "JSMN_ERROR_NOMEM", 
	    "JSMN_ERROR_INVAL", 
	    "JSMN_ERROR_PART"
	};
	lemonLog(ERROR, "jsmn faileld to parse json %s", 
		JSMN_ERR[numberTokens]);
	task->cleanTask = workspace_handleError;
	task->exitStatus = NON_FATAL;
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

    /*
     * Something is wrong when intermediate 
     * workspaces is not there at start i.e.,
     * 1,2,3 works but 1,3,4 wont' work properly...
     */
    switch(event){
	case INIT:
	    parseChangeinit(parser, token, numberTokens, ws);
	    break;
	case FOCUS:
	    parseChangefocus(parser, token, numberTokens, ws);
	    break;
	case EMPTY:
	    parseChangeempty(parser, token, numberTokens, ws);
	    break;
	default:
	    lemonLog(DEBUG, "Unhandled message");
	    break;
    }

    free(ws->internal->json);
    ws->internal->json = NULL;
    task->exitStatus = FINE;
}

private_ void workspace_formatWorkspace(
	struct taskRunner *task,
	void *_ws_)
{
    struct workspace *ws = _ws_;

    int currLen = 0;
    currLen += sprintf(ws->workspaceFormatted, "%%{l}");
    for(int i = 0; i < NUMBER_WORKSPACES; i++){
	if(ws->json[i].active != true){
	    continue;
	}
	if(ws->json[i].focused == true){
	    currLen += sprintf(
		    &ws->workspaceFormatted[0] + currLen, 
		    "%%{B#%s} %s %%{B-}", ws->focusedColor,
		    ws->json[i].name);
	} else {
	    currLen += 
		sprintf(&ws->workspaceFormatted[0] + currLen,
		       	"%%{B#%s} %s %%{B-}", ws->unfocusedColor,
			ws->json[i].name);
	} 
    }
}


private_ void workspace_handleError(
	struct taskRunner *task,
	void *_ws_)
{
    struct workspace *ws = _ws_;
    if(ws->internal->json){
	lemonLog(DEBUG, "Failed to parse a message clearing it now");
	free(ws->internal->json);
	ws->internal->lenjson = 0;
    }

    if(ws->fd != 0 && ws->internal->reconnect){
	lemonLog(DEBUG, "Reconnecting socket not ready - probably"
			"a failed connection attempt");
	close(ws->fd);
	ws->fd = -1;
    }
}

private_ void workspace_resetConnection(
	struct taskRunner *task,
	void *_ws_)
{
    struct workspace *ws = _ws_;
    if(ws->internal->json){
	lemonLog(DEBUG, "Clearing message during reconnect");
	free(ws->internal->json);
	ws->internal->lenjson = 0;
    }

    close(ws->fd);
    ws->fd = -1;
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
