#include <stdlib.h>
#include <poll.h>

#include "task-runner.h"
#include "lemonCommunication.h"
#include "workspace.h"

void runLoop(struct workspace *ws, struct lemonbar *lm)
{
    struct pollfd fds[10];
    nfds_t nfds = 1;
    fds[0].fd = ws->fd;
    fds[0].events = 0 | POLLIN;
    fds[1].fd = lm->stdout;
    fds[1].events = 0 | POLLIN;

    bool run = true;

    do {
	int read = poll(fds, nfds, 10);


	if(read > 0){
	    if((fds[0].revents & POLLIN) == POLLIN){
		ws->event(ws);
		lm->ws = ws;
		lm->com(lm);
	    }
	}
    } while(run);

}

int taskRunner_runTask(struct taskRunner task)
{
   while(task.nextTask != NULL){
       task.nextTask(&task, task.arg);
   }
   return task.exitStatus;
}
