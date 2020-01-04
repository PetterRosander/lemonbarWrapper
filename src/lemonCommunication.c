/******************************************************************************
 * PreProcessor directive
 *****************************************************************************/
#define _XOPEN_SOURCE 
#define __LEMON__
#include "lemonCommunication.h"

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
    task->nextTask[1] = lemon_formatWorkspace;
    task->nextTask[2] = lemon_formatNormal;
    task->nextTask[3] = lemon_sendLemonbar;
    task->nbrTasks = 4;
    task->arg = lm;
    taskRunner_runTask(task);
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
	task->nextTask[0] = lemon_formatWorkspace;
	task->nextTask[1] = lemon_formatNormal;
	task->nextTask[2] = lemon_sendLemonbar;
	task->nbrTasks = 3;
    }
	
    taskRunner_runTask(task);
}

private_ void lemon_reconfigure(
	struct taskRunner *task,
	struct lemonbar *lm){
    task->nextTask[0] = lemon_teardownCommunication;
    task->nextTask[1] = lemon_setupCommunication;
    task->nextTask[2] = lemon_formatWorkspace;
    task->nextTask[3] = lemon_formatNormal;
    task->nextTask[4] = lemon_sendLemonbar;
    task->nbrTasks = 5;
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
    (void)r;
    int i = system(strtok(action, "\n"));
    (void)i;
    task->exitStatus = 0;
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
	task->exitStatus = -1;
	return;
    }
    task->exitStatus = 0;
}

private_ void lemon_teardownCommunication(
	struct taskRunner *task,
	void *_lm_)
{
    struct lemonbar *lm = _lm_;
    int ret = pkill("lemonbar");
    close(lm->internal->pipeWrite);
    close(lm->pipeRead);
    (void) ret;
    task->exitStatus = 0;
}

private_ void lemon_formatWorkspace(
	struct taskRunner *task,
	void *_lm_)
{
    struct lemonbar *lm = _lm_;
    struct workspace *ws = lm->ws;

    int currLen = 0;
    currLen += sprintf(lm->internal->lemonFormat, "%%{c}");
    for(int i = 0; i < NUMBER_WORKSPACES; i++){
	if(ws->json[i].active != true){
	    continue;
	}
	if(ws->json[i].focused == true){
	    currLen += sprintf(
		    &lm->internal->lemonFormat[0] + currLen, 
		    "%%{+u}%%{U#0FF} %s %%{U-}%%{-u} | ", 
		    ws->json[i].name);
	} else if (i != NUMBER_WORKSPACES - 1) {
	    currLen += 
		sprintf(&lm->internal->lemonFormat[0] + currLen,
		       	"%s | ", ws->json[i].name);
	} 
    }
    sprintf(&lm->internal->lemonFormat[0] + currLen - 3, "\n");
    currLen -= 2;
    lm->internal->lenFormat = currLen;
    task->exitStatus = 0;
}

private_ void lemon_formatNormal(
	struct taskRunner *task,
	void *_lm_)
{
    struct lemonbar *lm = _lm_;
    struct plugins *pl = lm->pl;

    lm->internal->lenFormat += sprintf(
	    &lm->internal->lemonFormat[lm->internal->lenFormat - 1], 
	    "%%{r}%s", pl->pluginsFormatted);
    task->exitStatus = 0;
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
    task->exitStatus = 0;
}

private_ void lemon_sendLemonbar(
	struct taskRunner *task,
	void * _lm_)
{
    struct lemonbar *lm = _lm_;
    size_t sentBytes = WRITE(lm->internal->pipeWrite, 
	    lm->internal->lemonFormat, lm->internal->lenFormat);
    if(lm->internal->lenFormat != sentBytes){
	task->exitStatus = -1;
	return;
    }
    sentBytes = WRITE(lm->internal->pipeWrite, "\n", 1);
    (void)sentBytes;

    task->exitStatus = 0;
}
