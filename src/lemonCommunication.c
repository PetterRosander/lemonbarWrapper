/******************************************************************************
 * PreProcessor directive
 *****************************************************************************/
#define _XOPEN_SOURCE 
#define __LEMON__
#include "lemonCommunication.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "system-calls.h"

#include "task-runner.h"
#include "workspace.h"
#include "sys-utils.h"

#define LEMONBAR "lemonbar"

/******************************************************************************
 * exported functions declaration
 *****************************************************************************/
/*
 * Setup function used to access
 * the functionallity in this module
 */
struct lemonbar *lemon_init(struct configuration *cfg)
{
    struct lemonbar *lm = calloc(1, sizeof(struct lemonbar));

    if(lm == NULL){
	return NULL;
    }

    struct lemon_internal__ 
	*internal = calloc(1, sizeof(struct lemon_internal__));

    if(internal == NULL){
	free(lm);
	return NULL;
    }

    char *value = NULL;
    hashmap_get(cfg->mcfg.configMap, "LEMON_ARGS", (void**)(&value));
    lm->lmcfg.lemonArgs = value;

    lm->internal = internal;

    lm->setup = lemon_setup;
    lm->addFd = lemon_addFd;
    lm->render = lemon_reRender;
    lm->reconfigure = lemon_reconfigure;
    lm->action = lemon_action;

    return lm;
}

/*
 * Clears memory associated with the
 * lemon struct
 */
int lemon_destroy(struct lemonbar *lm)
{
    free(lm->internal);
    lm->internal = NULL;
    free(lm);
    lm = NULL;
    return 0;
}

/******************************************************************************
 * lemonCommunication entry functions
 *****************************************************************************/
private_ void lemon_setup(
	struct taskRunner *task,
	struct lemonbar *lm){
    task->nextTask[0] = lemon_setupCommunication;
    task->nextTask[1] = lemon_formatNormal;
    task->nextTask[2] = lemon_sendLemonbar;
    task->nbrTasks = 3;
    task->arg = lm;
    taskRunner_runTask(task);
}

private_ void lemon_addFd(
	struct taskRunner *task,
	struct lemonbar *lm)
{
    if(lm->pipeRead < 0){
	lemon_setup(task, lm);
    }

    if(lm->pipeRead >= 0){
	task->fds[task->nfds].fd = lm->pipeRead;
	task->fds[task->nfds].events = POLLIN;
	task->nfds++;
    }
}

private_ void lemon_reRender(
	struct taskRunner *task,
	struct lemonbar *lm){
    task->arg = lm;
    if(lm->pl->shutdownOrLock){
	task->nextTask[0] = lemon_lockOrShutdown;
	task->nextTask[1] = lemon_sendLemonbar;
	task->nbrTasks = 2;
    } else {
	task->nextTask[0] = lemon_formatNormal;
	task->nextTask[1] = lemon_sendLemonbar;
	task->nbrTasks = 2;
    }
	
    taskRunner_runTask(task);
}

private_ void lemon_reconfigure(
	struct taskRunner *task,
	struct lemonbar *lm){
    task->nextTask[0] = lemon_teardownCommunication;
    task->nextTask[1] = lemon_setupCommunication;
    task->nextTask[2] = lemon_formatNormal;
    task->nextTask[3] = lemon_sendLemonbar;
    task->nbrTasks = 4;
    task->arg = lm;
    taskRunner_runTask(task);
}

private_ void lemon_action(
	struct taskRunner *task,
	struct lemonbar *lm){
    task->nextTask[0] = lemon_pluginAction;
    task->nbrTasks = 1;
    task->arg = lm;
    taskRunner_runTask(task);
}


/******************************************************************************
 * Local function declarations
 *****************************************************************************/
private_ void lemon_pluginAction(
	struct taskRunner *task,
	void *_lm_)
{
    struct lemonbar *lm = _lm_;
    char action[100] = {0};
    int r = read(lm->pipeRead, action, sizeof(action));
    if(r < 0) {
	lemonLog(ERROR, "Failed to read command from lemonbar %s",
	       	strerror(errno));
	task->exitStatus = CRITICAL;
	task->cleanTask = lemon_handleError;
	return;
    }

    if(action[0] == 'x'){
	task->exitStatus = FINE;
	return;
    }

    int i = system(strtok(action, "\n"));

    if(i < 0) {
	lemonLog(ERROR, "Failed to run command %s - %s", action, 
		strerror(errno));
	task->exitStatus = DO_NOTHING;
	return;
    }

    task->exitStatus = FINE;
}

private_ void lemon_handleError(
	struct taskRunner *task,
	void *_lm_)
{
    struct lemonbar *lm = _lm_;
    close(lm->internal->pipeWrite);
    lm->internal->pipeWrite = -1;
    close(lm->pipeRead);
    lm->pipeRead = -1;

    lemon_setup(task, lm);
}

private_ void lemon_setupCommunication(
	struct taskRunner *task,
	void *_lm_)
{
    struct lemonbar *lm = _lm_;
    char lemonbar[1024] = {0};
    sprintf(lemonbar, "%s %s", LEMONBAR, lm->lmcfg.lemonArgs);
    lm->internal->pid = popen2(lemonbar, &lm->internal->pipeWrite,
	    &lm->pipeRead);

    if(lm->internal->pid < 0) {
	lemonLog(ERROR, "Failed to fork of lemonbar %s", strerror(errno));
	task->exitStatus = FATAL;
	return;
    }
    task->exitStatus = FINE;
}

private_ void lemon_teardownCommunication(
	struct taskRunner *task,
	void *_lm_)
{
    struct lemonbar *lm = _lm_;
    int ret = pkill("lemonbar");

    if(ret == -1){
	lemonLog(DEBUG, "lemonbar not running?");
    }

    int i = close(lm->internal->pipeWrite);
    if(i < 0){
	lemonLog(ERROR, "failed to close lemonbar pipe write - "
		"%s", strerror(errno));
    }
    lm->internal->pipeWrite = -1;
    i = close(lm->pipeRead);
    if(i < 0){
	lemonLog(ERROR, "failed to close lemonbar pipe read - "
		"%s", strerror(errno));
    }
    lm->pipeRead = -1;
    task->exitStatus = FINE;
}

private_ void lemon_formatNormal(
	struct taskRunner *task,
	void *_lm_)
{
    struct lemonbar *lm = _lm_;
    struct plugins *pl = lm->pl;
    struct workspace *ws = lm->ws;
    lm->internal->lenFormat = 0;

    lm->internal->lenFormat += sprintf(
	    lm->internal->lemonFormat, 
	    "%s %%{r}%s", ws->workspaceFormatted, pl->pluginsFormatted);
    task->exitStatus = FINE;
}

private_ void lemon_lockOrShutdown(
	struct taskRunner *task,
	void * _lm_)
{

    struct lemonbar *lm = _lm_;
    lm->internal->lenFormat = 0;
    lm->internal->lenFormat = 
	sprintf(lm->internal->lemonFormat, "%%{c}%%{A:i3lock-fancy:}\uf023%%{A}"
	       	" | %%{A:i3-msg exit:}\uf29d%%{A}"
	       	" | %%{A:shutdown -h now:}\uf011%%{A}"
		"%%{r}%%{A:x:}x%%{A}");
    task->exitStatus = FINE;
}

private_ void lemon_sendLemonbar(
	struct taskRunner *task,
	void * _lm_)
{
    struct lemonbar *lm = _lm_;
    size_t sentBytes = WRITE(lm->internal->pipeWrite, 
	    lm->internal->lemonFormat, lm->internal->lenFormat);
    if(sentBytes < 0){
	lemonLog(ERROR, "Failed to send message %s", strerror(errno));
	task->exitStatus = CRITICAL;
	task->cleanTask = lemon_handleError;
	return;
    }
    sentBytes = WRITE(lm->internal->pipeWrite, "\n", 1);
    if(sentBytes < 0){
        lemonLog(ERROR, "Failed to send line ending %s", strerror(errno));
        task->exitStatus = CRITICAL;
        task->cleanTask = lemon_handleError;
        return;
    }

    task->exitStatus = FINE;
}
