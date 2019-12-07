#ifndef _COMMCHANNEL_H_
#define _COMMCHANNEL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "workspace.h"

struct lemonbar
{
    FILE *fp;
    size_t lenFormat;
    char *lemonFormat;
};

int initChannel(struct lemonbar *lm);
int formatLemonBar(struct lemonbar *lm, struct workspace ws);
int sendlemonBar(struct lemonbar lm);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _COMMCHANNEL_H_ */
