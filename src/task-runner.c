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

#define MILLI 1000
/*
 * Snowflake try and catch error handling
 * for fun
 */
#define try bool __HadError=false; int e = 0;
#define catch(x) ExitJmp:if(__HadError)
#define throw(x) __HadError=true;goto ExitJmp
#define run(target) \
    target; \
    if(task->exitStatus < 0) { \
	e = task->exitStatus; \
	throw(); \
    }


extern bool main_exitRequested(void);

void runLoop(
	struct taskRunner *task,
	struct workspace *ws, 
	struct plugins *pl,
	struct configuration *cfg,
	struct lemonbar *lm)
{


    do {
	task->poll_t = 10*MILLI;
	task->nfds = 0;

	ws->addFd(task, ws);
	cfg->addFd(task, cfg);
	lm->addFd(task, lm);
	pl->addFd(task, pl);


	int readyfds = poll(task->fds, task->nfds, task->poll_t);

	if(readyfds > 0){
	    for(int i = 0; i < task->nfds; i++){
		if(ws->fd == task->fds[i].fd){
		    if((task->fds[i].revents & POLLHUP) == POLLHUP){
			ws->reconnect(task, ws);
		    } else if((task->fds[i].revents & POLLIN) == POLLIN){
			ws->event(task, ws);
		    }
		} else if (task->fds[i].fd == cfg->eventFd) {
		    if((task->fds[i].revents & POLLIN) == POLLIN){
			cfg->event(task, cfg);
			lm->reconfigure(task, lm);
			pl->reconfigure(task, pl);
		    }
		} else if(task->fds[i].fd == lm->pipeRead){
		    if((task->fds[i].revents & POLLIN) == POLLIN){
			lm->action(task, lm);
			pl->shutdownOrLock = false;
		    }
		} else if(task->fds[i].fd == pl->pluginsFd){
		    if((task->fds[i].revents & POLLIN) == POLLIN){
			pl->event(task, pl);
		    }
		}
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
	    run(task->nextTask[i](task, task->arg));
	}
    } catch(e) {
	switch(e){
	    case DO_NOTHING:
		lemonLog(DEBUG, "DO_NOTHING: Recived error")
		    break;
	    case NON_FATAL:
		lemonLog(DEBUG, "NON_FATAL: running clean up");
		task->cleanTask(task, task->arg);
		break;
	    case CRITICAL:
		lemonLog(DEBUG, "CRITICAL: running cleanTask");
		task->cleanTask(task, task->arg);
		break;
	    case FATAL:
		lemonLog(DEBUG, "FATAL: Recived no recoverable error exiting");
		exit(EXIT_FAILURE);
		break;
	    default:
		break;
	}
    }
    return e;
}
