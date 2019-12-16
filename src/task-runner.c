#include <stdlib.h>
#include <poll.h>

#include "task-runner.h"
#include "commChannel.h"
#include "workspace.h"

void runLoop(struct workspace *ws, struct lemonbar *lm)
{
    struct pollfd fds[10];
    nfds_t nfds = 1;
    fds[0].fd = ws->fd;
    fds[0].events = 0 | POLLIN;

    int read = poll(fds, nfds, 10);


    if(read > 0){
	if((fds[0].revents & POLLIN) == POLLIN){
	}
    }

}

int taskRunner_runTask(struct taskRunner task)
{
   while(task.nextTask != NULL){
       task.nextTask(&task, task.arg);
   }
   return task.exitStatus;
}
