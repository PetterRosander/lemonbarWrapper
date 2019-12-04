#define _XOPEN_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commChannel.h"
#include "i3query.h"

#define LEMONBAR "lemonbar -u 2"


static void formatWorkspace(struct workspace ws, char *out);

int initChannel(struct lemonbar *lm)
{
    lm->fp = popen(LEMONBAR, "w");
    if(lm->fp == NULL) {
	return 0;
    }

    return 0;
}

int formatLemonBar(struct lemonbar *lm, struct workspace ws)
{
    char out[1024] = {0};

    formatWorkspace(ws, out);
    lm->lenFormat = strlen(out) + 1;
    lm->lemonFormat = calloc(sizeof(char), lm->lenFormat);

    if(lm->lemonFormat == NULL){
	return -1;
    }

    char *ptr = strncpy(lm->lemonFormat, out, lm->lenFormat);
    if(ptr == NULL){
	return -2;
    }

    return 0;
}


int sendlemonBar(struct lemonbar lm)
{
    size_t sentBytes = fprintf(lm.fp, "%s\n", lm.lemonFormat);
    if(lm.lenFormat != sentBytes){
	return -1;
    }
    fflush(lm.fp);
    return 0;
}


static void formatWorkspace(struct workspace ws, char *out)
{
    int currLen = 0;
    currLen += sprintf(out, "%%{c}");
    for(int i = 0; i < ws.numberws; i++){
	if(ws.json[i].num == 0){
	    continue;
	}
	if(ws.json[i].focused == true){
	    currLen += sprintf(
		    &out[0] + currLen, 
		    "%%{+o}%%{+u}%%{U#099}%s%%{U-}%%{-u}%%{-o} | ", 
		    ws.json[i].name);
	} else {
	    currLen += sprintf(&out[0] + currLen, "%s | ", ws.json[i].name);
	}
    }
    if(ws.json[ws.numberws].focused == true){
	currLen += sprintf(
		&out[0] + currLen, 
		"%%{+o}%%{+u}%%{U#099}%s%%{U-}%%{-u}%%{-o}", 
		ws.json[ws.numberws].name);
    } else {
	currLen += sprintf(&out[0] + currLen, "%s", ws.json[ws.numberws].name);
    }

}
