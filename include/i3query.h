#ifndef _I3QUERY_H_
#define _I3QUERY_H_
#include <sys/types.h>
#include <stdbool.h>
#include <stdint.h>

#include "i3query.h"

struct parsedJsonInformation {
    bool focused;
    char name[128];
    int num;
};

struct workspace 
{
    int fd;
    char json_i3[1024*4];
    size_t lenjson_i3;
    struct parsedJsonInformation json[10];
    uint8_t numberws;
};


int subscribeWorkSpace(struct workspace *ws, char *i3path);
int eventi3(struct workspace *ws);
int startWorkSpace(struct workspace *ws);
int jsonParseMessageCommand(struct workspace *ws);
int jsonParseMessageEvent(struct workspace *ws);

#endif
