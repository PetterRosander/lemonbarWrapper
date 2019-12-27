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

#define LEMONBAR "lemonbar"

#define PIPE_READ 0
#define PIPE_WRITE 1

/******************************************************************************
 * inlined function declarations
 *****************************************************************************/
static inline pid_t popen2(
	const char *,
	int *,
	int *);

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
private_ void lemon_setup(struct lemonbar *lm){
    struct taskRunner task = {0};
    task.nextTask = lemon_setupCommunication;
    task.arg = lm;
    taskRunner_runTask(task);
}

private_ void lemon_reRender(struct lemonbar *lm){
    struct taskRunner task = {0};
    task.arg = lm;
    if(lm->pl->shutdownOrLock){
	task.nextTask = lemon_lockOrShutdown;
    } else {
	task.nextTask = lemon_formatWorkspace;
    }
	
    taskRunner_runTask(task);
}

private_ void lemon_reconfigure(struct lemonbar *lm){
    struct taskRunner task = {0};
    task.nextTask = lemon_teardownCommunication;
    task.arg = lm;
    taskRunner_runTask(task);
}

private_ void lemon_action(struct lemonbar *lm){
    struct taskRunner task = {0};
    task.nextTask = lemon_pluginAction;
    task.arg = lm;
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
    task->nextTask = NULL;
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
	task->nextTask   = NULL;
	return;
    }
    task->exitStatus = 0;
    task->nextTask = lemon_formatWorkspace;
}

private_ void lemon_teardownCommunication(
	struct taskRunner *task,
	void *_lm_)
{
    struct lemonbar *lm = _lm_;
    close(lm->internal->pipeWrite);
    close(lm->pipeRead);
    int ret = system("pkill lemonbar");
    int status = 0;
    waitpid(ret, &status, 0);
    task->exitStatus = 0;
    task->nextTask = lemon_setupCommunication;
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
    task->nextTask   = lemon_formatNormal;
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
    task->nextTask   = lemon_sendLemonbar;
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
    task->nextTask   = lemon_sendLemonbar;
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
	task->nextTask   = NULL;
	return;
    }
    sentBytes = WRITE(lm->internal->pipeWrite, "\n", 1);
    (void)sentBytes;

    task->exitStatus = 0;
    task->nextTask = NULL;
}

/******************************************************************************
 * Inlined small functions (should only be used once)
 *****************************************************************************/
/*
 * Since popen does not support bidirectional
 * communication we write our own popen 
 */
static inline pid_t popen2(
	const char *command,
	int *infp,
	int *outfp)
{
    int p_stdin[2], p_stdout[2];

    pid_t pid;

    if (pipe(p_stdin) != 0 || pipe(p_stdout) != 0){
	return -1;
    }

    pid = fork();

    if (pid < 0){
	return pid;
    } else if (pid == 0){
	close(p_stdin[PIPE_WRITE]);
	dup2(p_stdin[PIPE_READ], PIPE_READ);
	close(p_stdout[PIPE_READ]);
	dup2(p_stdout[PIPE_WRITE], PIPE_WRITE);
	execl("/bin/sh", "sh", "-c", command, NULL);
	perror("execl");
	exit(1);
    }

    if (infp == NULL){
	close(p_stdin[PIPE_WRITE]);
    } else{
	*infp = p_stdin[PIPE_WRITE];
    }

    if (outfp == NULL){
	close(p_stdout[PIPE_READ]);
    } else {
	*outfp = p_stdout[PIPE_READ];
    }

    return pid;
}
