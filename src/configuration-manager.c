/******************************************************************************
 * PreProcessor directive
 *****************************************************************************/
#define __CONFIGURATION__
#include "configuration-manager.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <string.h>

#include "task-runner.h"
#include "hashmap.h"
/******************************************************************************
 * exported functions declaration
 *****************************************************************************/
/* 
 * Setup function used to access the 
 * functionallity in this modue
 */
struct configuration *config_init(const char* path)
{
    struct configuration *cfg = calloc(1, sizeof(struct configuration));
    if(cfg == NULL){
	return NULL;
    }

    strcpy(cfg->configPath, path);
    cfg->setup = config_setup;
    cfg->event = config_event;
    return cfg;

}

/*
 * Clear memory assiciated with the
 * confugration struct
 */
int config_destory(struct configuration *cfg)
{
    hashmap_free(cfg->mcfg.configMap);
    free(cfg);
    cfg = NULL;
    return 0;
}

/******************************************************************************
 * lemonCommunication entry functions
 *****************************************************************************/
private_ void config_setup(struct configuration *cfg)
{
    struct taskRunner task = {0};
    task.nextTask = config_addWatcher;
    task.arg = cfg;
    taskRunner_runTask(task);
}

private_ void config_event(struct configuration *cfg)
{
    struct taskRunner task = {0};
    task.nextTask = config_handleEvents;
    task.arg = cfg;
    taskRunner_runTask(task);
}

/******************************************************************************
 * Local function declarations
 *****************************************************************************/
private_ void config_addWatcher(
	struct taskRunner *task,
	void *_cfg_)
{

    struct configuration *cfg = _cfg_;
    cfg->eventFd = inotify_init1(IN_NONBLOCK);

    if (cfg->eventFd == -1) {
	task->exitStatus = -1;
	task->nextTask = NULL;
	return;
    }

    int wd = inotify_add_watch(cfg->eventFd, cfg->configPath, 
	    IN_CLOSE_WRITE);

    if (wd == -1) {
	task->exitStatus = -1;
	task->nextTask = NULL;
    }
    task->nextTask = config_readConfiguration;
    task->exitStatus = 0;
}

private_ void config_handleEvents(
	struct taskRunner *task,
	void *_cfg_)
{
    struct configuration *cfg = _cfg_;
    struct inotify_event event = {0};

    ssize_t len = read(cfg->eventFd, (void*)&event, 
	    sizeof(struct inotify_event));
    
    if (len != sizeof(struct inotify_event)){
	task->exitStatus = -1;
	task->nextTask = NULL;
	return;
    }

    if (event.mask & IN_CLOSE_WRITE){
	task->exitStatus = 0;
	task->nextTask = config_readConfiguration;
	return;
    }

    task->exitStatus = 0;
    task->nextTask = NULL;
}


private_ void config_readConfiguration(
	struct taskRunner *task,
	void *_cfg_)
{
    struct configuration *cfg = _cfg_;
    FILE *fcfg = fopen(cfg->configPath, "r");
    if(fcfg == NULL){
	task->nextTask = NULL;
	task->exitStatus = -1;
	return;
    }

    if(cfg->mcfg.configMap == NULL){
	cfg->mcfg.configMap = hashmap_new();
    }


    char line[1000] = {0};
    for(int i = 0; i < NCONFIG_PARAM && 
	    NULL != fgets(line, sizeof(line), fcfg); i++){
	if(line[i] == '#') continue;
	char *tok = &line[0];
	char *nextTok = tok;
	while(*nextTok != '=') nextTok++;
	*nextTok = '\0';
	nextTok++;
	strcpy(cfg->mcfg.key[i], tok);
	strcpy(cfg->mcfg.value[i], nextTok);
	int error = hashmap_put(cfg->mcfg.configMap, cfg->mcfg.key[i], (void *)cfg->mcfg.value[i]);	
	(void) error;
    }

    task->exitStatus = 0;
    task->nextTask = NULL;
    fclose(fcfg);
}
