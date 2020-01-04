/******************************************************************************
 * PreProcessor directive
 *****************************************************************************/
#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include <unistd.h>

#include "task-runner.h"
#include "plugins.h"
#include "lemonCommunication.h"
#include "workspace.h"

/*
 * Snowflake try and catch error handling
 */
#define try bool __HadError=false;
#define catch(x) ExitJmp:if(__HadError)
#define throw(x) __HadError=true;goto ExitJmp;
#define MILLI 1000

extern bool main_exitRequested(void);

void runLoop(
	struct taskRunner *task,
	struct workspace *ws, 
	struct plugins *pl,
	struct configuration *cfg,
	struct lemonbar *lm)
{
    struct pollfd fds[10];
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
		    usleep(100000);
		    ws->reconnect(task, ws);
		    fds[0].fd = ws->fd;
		} else if((fds[0].revents & POLLIN) == POLLIN){
		    ws->event(task, ws);
		}
	    }
	    if((fds[1].revents & POLLIN) == POLLIN){
		cfg->event(task, cfg);
		pl->reconfigure(task, pl);
		lm->reconfigure(task, lm);
	    }
	    if((fds[2].revents & POLLIN) == POLLIN){
		lm->action(task, lm);
		pl->shutdownOrLock = false;
	    }
	    if((fds[3].revents & POLLIN) == POLLIN){
		pl->event(task, pl);
	    }
	}

	pl->normal(task, pl);
	lm->render(task, lm);

    } while(!main_exitRequested());

}

int taskRunner_runTask(struct taskRunner *task)
{
    try {
	for(int i = 0; i < task->nbrTasks; i++){
	    task->nextTask[i](task, task->arg);
	    if(task->exitStatus == -1) {
		throw();
	    }
	}
    } catch(...) {
	printf("Caught error\n");
    }
    return 0;
}
