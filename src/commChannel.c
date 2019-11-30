#define _XOPEN_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commChannel.h"

#define LEMONBAR "~/Downloads/bar/lemonbar"

struct lemonbar initChannel()
{
    struct lemonbar lm = {0};

    lm.fp = popen(LEMONBAR, "w");
    if(lm.fp == NULL) {
	exit(EXIT_FAILURE);
    }

    return lm;
}

int formatLemonBar(struct lemonbar *lm, char *str)
{
    lm->lenFormat = strlen(str) + 1;
    lm->lemonFormat = calloc(sizeof(char), lm->lenFormat);

    if(lm->lemonFormat == NULL){
	return -1;
    }

    char *ptr = strncpy(lm->lemonFormat, str, lm->lenFormat);
    if(ptr == NULL){
	return -2;
    }

    return 0;
}


int sendlemonBar(struct lemonbar lm)
{
    size_t sentBytes = fprintf(lm.fp, "%s\n", lm.lemonFormat);
    if(lm.lenFormat != sentBytes){
	fprintf(stderr, "sentBytes: %lu expected: %lu\nformat %s\n", sentBytes, lm.lenFormat, lm.lemonFormat);
	return -1;
    }
    fflush(lm.fp);
    return 0;
}
