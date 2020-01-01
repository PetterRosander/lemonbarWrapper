/******************************************************************************
 * PreProcessor directive
 *****************************************************************************/
#include <stdlib.h>
#include <poll.h>
#include <string.h>

#include <unistd.h>

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
    bool i3Restart = false;
    nfds_t nfds = 4;
    fds[0].fd = ws->fd;
    fds[0].events = POLLIN;
    fds[0].events |= POLLHUP;
    fds[1].fd = cfg->eventFd;
    fds[1].events = POLLIN;
    fds[2].fd = lm->pipeRead;
    fds[2].events = POLLIN;
    fds[3].fd = pl->pluginsFd;
    fds[3].events = POLLIN;


    do {
	int readyfds = poll(fds, nfds, 10*MILLI);


	if(readyfds > 0){
	    if(fds[0].revents){
		if((fds[0].revents & POLLHUP) == POLLHUP){
		    ws->reconnect(ws);
		    fds[0].fd = ws->fd;
		} else if((fds[0].revents & POLLIN) == POLLIN){
		    ws->event(ws);
		}
	    }
	    if((fds[1].revents & POLLIN) == POLLIN){
		cfg->event(cfg);
		pl->reconfigure(pl);
		lm->reconfigure(lm);
	    }
	    if((fds[2].revents & POLLIN) == POLLIN){
		lm->action(lm);
		pl->shutdownOrLock = false;
	    }
	    if((fds[3].revents & POLLIN) == POLLIN){
		pl->event(pl);
	    }
	}

	pl->normal(pl);
	lm->render(lm);

    } while(!main_exitRequested() && !i3Restart);

}

int taskRunner_runTask(struct taskRunner task)
{
   while(task.nextTask != NULL){
       task.nextTask(&task, task.arg);
   }
   return task.exitStatus;
}
