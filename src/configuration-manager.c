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
#include "sys-utils.h"
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
private_ void config_setup(
	struct taskRunner *task,
	struct configuration *cfg)
{
    task->nextTask[0] = config_addWatcher;
    task->nextTask[1] = config_readConfiguration;
    task->nbrTasks = 2;
    task->arg = cfg;
    taskRunner_runTask(task);
}

private_ void config_event(
	struct taskRunner *task,
	struct configuration *cfg)
{
    task->nextTask[0] = config_handleEvents;
    task->nextTask[1] = config_readConfiguration;
    task->nbrTasks = 2;
    task->arg = cfg;
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
	lemonLog(ERROR, "Failed to create inofity fd %s", 
			strerror(errno));
	task->exitStatus = -1;
	return;
    }

    int wd = inotify_add_watch(cfg->eventFd, cfg->configPath, 
	    IN_CLOSE_WRITE);

    if (wd == -1) {
	lemonLog(ERROR, "Failed to add watcher inofity fd %s", 
			strerror(errno));
	task->exitStatus = -1;
	return;
    }

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
	lemonLog(ERROR, "Failed to read inotify event - read size: %llu"
	    "inotify_event size: %llu\n", len, sizeof(struct inotify_event));
	task->exitStatus = -1;
	return;
    }

    if (!(event.mask & IN_CLOSE_WRITE)){
	lemonLog(DEBUG, "Did not recive IN_CLOSE_WRITE - unexpected");
	task->exitStatus = 0;
	return;
    }

    task->exitStatus = 0;
}


private_ void config_readConfiguration(
	struct taskRunner *task,
	void *_cfg_)
{
    struct configuration *cfg = _cfg_;
    FILE *fcfg = fopen(cfg->configPath, "r");
    if(fcfg == NULL){
	lemonLog(DEBUG, "Failed to open configuration %s", cfg->configPath);
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
	int error = hashmap_put(cfg->mcfg.configMap, 
		cfg->mcfg.key[i], (void *)cfg->mcfg.value[i]);	
	(void) error;
    }

    task->exitStatus = 0;
    fclose(fcfg);
}
