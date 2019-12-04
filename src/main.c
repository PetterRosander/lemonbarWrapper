#define _XOPEN_SOURCE
#include <unistd.h>
#include <string.h>

#include "commChannel.h"
#include "i3query.h"

#define ERROR(error) \
    fprintf(stderr, "%s\n", error); \
    exit(EXIT_FAILURE);

static void runLoop(struct workspace *ws, struct lemonbar *lm);

int main(int argc, char *argv[])
{

    int opt;
    char i3path[128] = {0};

    // TODO: Add long opts (see manual getopt)
    while((opt = getopt(argc, argv, "p:")) != -1) {
	switch(opt){
	case 'p':
	    strcpy(i3path, optarg);
	    printf("%s\n", i3path);
	    break;
	default:
	    printf("Unknown option"); 
	    break;
	}
    }

    struct lemonbar lm = {0};
    if(initChannel(&lm) != 0){
	perror("setting up bar");
    }

    
    struct workspace ws = {0};
    if(subscribeWorkSpace(&ws, i3path) != 0){
	perror("subscribe error");
    }
    
    if(startWorkSpace(&ws) != 0){
	perror("subscribe error");
    }

    jsonParseMessageCommand(&ws);
    
    if(formatLemonBar(&lm, ws) != 0){
	perror("setting up bar");
    }

    if(sendlemonBar(lm)!= 0){
	perror("send error ");
    }

    while(true){
	runLoop(&ws, &lm);
    }

    return 0;
}

#include <poll.h>
static void runLoop(struct workspace *ws, struct lemonbar *lm)
{
    struct pollfd fds[10];
    nfds_t nfds = 1;
    fds[0].fd = ws->fd;
    fds[0].events = 0 | POLLIN;

    int read = poll(fds, nfds, 10);


    if(read > 0){
	if((fds[0].revents & POLLIN) == POLLIN){
	    if(eventi3(ws) != 0){
		perror("subscribe error");
	    }

	    jsonParseMessageEvent(ws);
	    if(formatLemonBar(lm, *ws) != 0){
		perror("setting up bar");
	    }

	    if(sendlemonBar(*lm)!= 0){
		perror("send error ");
	    }


	}
    }

}

