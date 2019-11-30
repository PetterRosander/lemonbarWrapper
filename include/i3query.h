#ifndef _I3QUERY_H_
#define _I3QUERY_H_

struct workspace 
{
    int fd;
};

int subscribeWorkSpace(struct workspace *ws, char *i3path);
int eventi3(struct workspace *ws);
int startWorkSpace(struct workspace *ws);

#endif
