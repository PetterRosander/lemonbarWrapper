#define _XOPEN_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

#include "i3query.h"


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

    char info[1024*4] = {0};
    i = read(ws->fd, &info[0], len);
    printf("%s\n", info);

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

    char info[1024*4] = {0};
    i = read(ws->fd, &info[0], len);
    printf("%s\n", info);


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
