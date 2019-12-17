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

#include "system-calls.h"

#include "task-runner.h"
#include "workspace.h"

#define LEMONBAR "lemonbar -u 2"

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
struct lemonbar *lemon_init(void)
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

    lm->internal = internal;

    lm->setup = lemon_setup;
    lm->com   = lemon_com;

    return lm;
}

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

private_ void lemon_com(struct lemonbar *lm){
    struct taskRunner task = {0};
    task.nextTask = lemon_action;
    task.arg = lm;
    taskRunner_runTask(task);
}


/******************************************************************************
 * Local function declarations
 *****************************************************************************/
private_ void lemon_setupCommunication(
	struct taskRunner *task,
	void *_lm_)
{
    struct lemonbar *lm = _lm_;
    lm->internal->pid = popen2(LEMONBAR, &lm->internal->pipeWrite,
	    &lm->pipeRead);

    if(lm->internal->pid < 0) {
	task->exitStatus = -1;
	task->nextTask   = NULL;
	return;
    }
    lm->action = WORKSPACE;
    task->exitStatus = 0;
    task->nextTask = lemon_action;
}


private_ void lemon_action(
	struct taskRunner *task,
	void *_lm_)
{
    struct lemonbar *lm = _lm_;
    switch(lm->action){
	case WORKSPACE:
	    task->nextTask   = lemon_formatWorkspace;
	    task->exitStatus = 0;
	    break;
	default:
	    task->nextTask = NULL;
	    task->exitStatus = -1;
	    break;
    }
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
		    "%%{+o}%%{+u}%%{U#099}%s%%{U-}%%{-u}%%{-o} | ", 
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

#if 0
int formatLemonBar(struct lemonbar *lm, struct workspace ws)
{
    char out[1024] = {0};

    formatWorkspace(ws, out);
    lm->lenFormat = strlen(out) + 1;
    lm->lemonFormat = calloc(sizeof(char), lm->lenFormat);

    if(lm->lemonFormat == NULL){
	return -1;
    }

    char *ptr = strncpy(lm->lemonFormat, out, lm->lenFormat);
    if(ptr == NULL){
	return -2;
    }

    return 0;
}
#endif
