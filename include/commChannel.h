#ifndef _COMMCHANNEL_H_
#define _COMMCHANNEL_H_
#include <stdio.h>

struct lemonbar
{
    FILE *fp;
    size_t lenFormat;
    char *lemonFormat;
};

int initChannel(struct lemonbar *lm);
int formatLemonBar(struct lemonbar *lm, char *str);
int sendlemonBar(struct lemonbar lm);


#endif
