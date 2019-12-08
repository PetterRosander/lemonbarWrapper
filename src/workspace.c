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

#include "workspace.h"

/******************************************************************************
 * exported functions declaration
 *****************************************************************************/
struct workspace *workspace_init(char *i3path)
{
    struct workspace *ws = calloc(1, sizeof(struct workspace)); 
    struct workspace_internal__
	*internal = calloc(1, sizeof(struct workspace_internal__));
    
    ws->event = eventWorkspace__;
    ws->internal = internal;
    printf("%p\n", (void *)(ws)->internal);


    if(setupSocket(ws, i3path) != 0){
	free(ws->internal);
	free(ws);

        return NULL;
    }

    if(subscribeWorkspace(ws) != 0){
	free(ws->internal);
	free(ws);
        return NULL;
    }

    startWorkSpace(ws);
    return ws;
}

int workspace_destroy(struct workspace *ws)
{
    free(ws->internal);
    ws->internal = NULL;

    free(ws);
    ws = NULL;
    return 0;
}


/******************************************************************************
 * Inlined functions used by the wrapper functions
 *****************************************************************************/
workspacePrivate int eventWorkspace__(struct workspace *ws)
{
    unsigned char event[14] = {0};

    int i = read(ws->fd, &event[0], sizeof(event));
    
    if(i < 0){
	return -1;
    }

    uint32_t len = 0;
    len += (uint32_t)event[6] << (8*0);
    len += (uint32_t)event[7] << (8*1);

    ws->internal->lenjson_i3 = read(ws->fd, (&(ws->internal->json_i3))[0], len);

    return 0;
}

/******************************************************************************
 * Local function declarations
 *****************************************************************************/
static int setupSocket(struct workspace *ws, char *i3path)
{
    int i3Sock = socket(PF_LOCAL, SOCK_STREAM, 0);
    ws->fd = i3Sock;
    if(i3Sock < 0){
	return -1;
    }

    struct sockaddr_un addr = {0}; 
    addr.sun_family = AF_LOCAL;
    strncpy(addr.sun_path, i3path, strlen(i3path));

    if(connect(i3Sock, (const struct sockaddr *)&addr, sizeof(addr)) < 0){
	return -1;
    }
    return 0;
}


static int subscribeWorkspace(struct workspace *ws)
{

    unsigned char subscribe[14];
    formatMessage(SUBSCRIBE, subscribe);
    if(write(ws->fd, subscribe, sizeof(subscribe)) == 0){
	return -1;
    }

    char event[15] = "[ \"workspace\" ]";
    if(write(ws->fd, event, sizeof(event)) == 0){
	return -1;
    }

    unsigned char reply[20] = {0};
    int i = read(ws->fd, &reply[0], 14);
    if(i < -1){
	return -1;
    }

    i = read(ws->fd, &reply[0], reply[6]);
    return 0;
}


static int startWorkSpace(struct workspace *ws)
{
    unsigned char request[14] = {0};
    
    formatMessage(COMMAND, request);
    if(write(ws->fd, request, sizeof(request)) == 0){
	return -1;
    }

    unsigned char reply[14] = {0};
    int i = read(ws->fd, &reply[0], sizeof(reply));

    if(i < 0){
	return -1;
    }
    
    uint32_t len = 0;
    len += (uint32_t)reply[6] << (8*0);
    len += (uint32_t)reply[7] << (8*1);

    ws->internal->lenjson_i3 = read(ws->fd, &(ws->internal->json_i3[0]), len);

    return 0;
}


static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
	    strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
	return 0;
    }
    return -1;
}


int jsonParseMessageEvent(struct workspace *ws)
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

int jsonParseMessageCommand(struct workspace *ws)
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

static void parseChangefocus(
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

static void parseChangeinit(jsmn_parser parser, jsmntok_t *token, 
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


static void formatMessage(enum I3_TYPE type, unsigned char *packet)
{
    memcpy(&packet[0], "i3-ipc", sizeof("i3-ipc"));
    switch(type){
	case SUBSCRIBE:
	    packet[6] = 15;
	    packet[7] = 0;
	    packet[8] = 0;
	    packet[9] = 0;
	    packet[10] = 2;
	    packet[11] = 0;
	    packet[12] = 0;
	    packet[13] = 0;
	    break;
	case COMMAND:
	    packet[6] = 0;
	    packet[7] = 0;
	    packet[8] = 0;
	    packet[9] = 0;
	    packet[10] = 1;
	    packet[11] = 0;
	    packet[12] = 0;
	    packet[13] = 0;
	    break;
	default:
	    ; // pass
	    break;
    }

}
