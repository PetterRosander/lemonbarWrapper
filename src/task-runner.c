/******************************************************************************
 * PreProcessor directive
 *****************************************************************************/
#include <stdlib.h>
#include <poll.h>
#include <string.h>

#include "task-runner.h"
#include "plugins.h"
#include "lemonCommunication.h"
#include "workspace.h"

#define MILLI 1000

extern bool main_exitRequested(void);

void runLoop(
	struct workspace *ws, 
	struct plugins *pl,
	struct configuration *cfg,
	struct lemonbar *lm)
{
    struct pollfd fds[10];
    nfds_t nfds = 2;
    fds[0].fd = ws->fd;
    fds[0].events = POLLIN;
    fds[0].events |= POLLHUP;
    fds[1].fd = cfg->eventFd;
    fds[1].events = POLLIN;
    fds[2].fd = lm->pipeRead;
    fds[2].events = POLLIN;


    do {
	int read = poll(fds, nfds, 10*MILLI);
	pl->normal(pl);


	if(read > 0){
	    if((fds[0].revents & POLLIN) == POLLIN){
		ws->event(ws);
	    }
	    if((fds[0].revents & POLLHUP) == POLLHUP){
		printf("i3 reloaded\n");
		// Possible reload of i3 - this will restart
		// lemonwrapper and we will end up with two
		// bars and poll endlessly returning with no timeout
		exit(0);
	    }
	    if((fds[1].revents & POLLIN) == POLLIN){
		cfg->event(cfg);
		lm->reconfigure(lm);
	    }
	    if((fds[2].revents & POLLIN) == POLLIN){
		// NOT IMPLEMENTED YET
	    }
	}

	lm->render(lm);
    } while(!main_exitRequested());

}

int taskRunner_runTask(struct taskRunner task)
{
   while(task.nextTask != NULL){
       task.nextTask(&task, task.arg);
   }
   return task.exitStatus;
}
