#define _XOPEN_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>

#include "i3query.h"

#define NBR_JSMN_TOKENS 1024
#define JSMN_STATIC
#include "jsmn.h"


enum I3_TYPE {
    COMMAND = 0,
    SUBSCRIBE = 1
};

static void formatMessage(enum I3_TYPE type, unsigned char *packet);


int subscribeWorkSpace(struct workspace *ws, char *i3path)
{
    int i3Sock = socket(PF_LOCAL, SOCK_STREAM, 0);
    if(i3Sock < 0){
	return -1;
    }

    struct sockaddr_un addr = {0}; 
    addr.sun_family = AF_LOCAL;
    strncpy(addr.sun_path, i3path, strlen(i3path));

    if(connect(i3Sock, (const struct sockaddr *)&addr, sizeof(addr)) < 0){
	return -1;
    }
    ws->fd = i3Sock;

    unsigned char subscribe[14];
    formatMessage(SUBSCRIBE, subscribe);
    if(write(i3Sock, subscribe, sizeof(subscribe)) == 0){
	return -1;
    }

    char event[15] = "[ \"workspace\" ]";
    if(write(i3Sock, event, sizeof(event)) == 0){
	return -1;
    }

    unsigned char reply[20] = {0};
    int i = read(i3Sock, &reply[0], 14);
    if(i < -1){
	return -1;
    }

    i = read(i3Sock, &reply[0], reply[6]);
    return 0;
}

int eventi3(struct workspace *ws)
{
    unsigned char event[14] = {0};

    int i = read(ws->fd, &event[0], sizeof(event));
    
    if(i < 0){
	return -1;
    }

    uint32_t len = 0;
    len += (uint32_t)event[6] << (8*0);
    len += (uint32_t)event[7] << (8*1);

    ws->lenjson_i3 = read(ws->fd, (&(ws->json_i3))[0], len);

    return 0;
}

int startWorkSpace(struct workspace *ws)
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

    ws->lenjson_i3 = read(ws->fd, (&(ws->json_i3))[0], len);

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

    int ret = jsmn_parse(&parser, ws->json_i3, ws->lenjson_i3, token, NBR_JSMN_TOKENS);

    if(ret < 1){
	return -1;
    }
    
    if(jsoneq(ws->json_i3, &token [1], "change") != 0 ||
       jsoneq(ws->json_i3, &token [2], "focus") != 0){
	return -1;
    }

    bool current = true;
    int  num = 0;
    for(int i = 0; i < ret; i++){
	if(token[i].type == JSMN_OBJECT && jsoneq(ws->json_i3, &token[i - 1], "old") == 0){
	    current = false;
	}
	if(jsoneq(ws->json_i3, &token[i], "num") == 0){
	    char value[10] = {0};
	    strncpy(value, ws->json_i3 + token[i + 1].start, token[i + 1].end - token[i + 1].start);
	    num = strtol(value, NULL, 10) - 1;
	    ws->json[num].focused = current ? true: false;
	    i++;
	} 
    }

    return 0;
}

int jsonParseMessageCommand(struct workspace *ws)
{
    jsmn_parser parser;
    jsmntok_t token[NBR_JSMN_TOKENS];

    jsmn_init(&parser);

    int ret = jsmn_parse(&parser, ws->json_i3, ws->lenjson_i3, token, NBR_JSMN_TOKENS);

    if(ret < 1){
       return -1;
    }

    int n = -1;
    for(int i = 0; i < ret; i++){
	if(token[i].type == JSMN_OBJECT && jsoneq(ws->json_i3, &token[i - 1], "rect") != 0){
	    n++;
	}
	if(jsoneq(ws->json_i3, &token[i], "focused") == 0){
	    char boolean[6] = {0};
	    strncpy(boolean, ws->json_i3 + token[i + 1].start, token[i + 1].end - token[i + 1].start);
	    if(strcmp(boolean, "true") == 0){
		ws->json[n].focused = true;
	    } else {
		ws->json[n].focused = false;
	    }
	    i++;
	} else if(jsoneq(ws->json_i3, &token[i], "name") == 0){
	    strncpy(ws->json[n].name, ws->json_i3 + token[i + 1].start, token[i + 1].end - token[i + 1].start);
	    i++;
	} else if(jsoneq(ws->json_i3, &token[i], "num") == 0){
	    char value[10] = {0};
	    strncpy(value, ws->json_i3 + token[i + 1].start, token[i + 1].end - token[i + 1].start);
	    ws->json[n].num = strtol(value, NULL, 10);
	    i++;
	}
    }

    ws->numberws = n;
    return 0;
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
