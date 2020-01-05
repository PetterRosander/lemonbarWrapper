/******************************************************************************
 * PreProcessor directive
 *****************************************************************************/
#define __CONFIGURATION__
#include "configuration-manager.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <string.h>

#include "task-runner.h"
#include "hashmap.h"
#include "sys-utils.h"


static inline void config_parseConfigValue(
	char *, 
	char *, 
	char *, 
	char *, 
	ssize_t, 
	ssize_t);
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
    cfg->addFd = config_addFd;
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

private_ void config_addFd(
	struct taskRunner *task,
	struct configuration *cfg)
{
    if(cfg->eventFd < 0){
	lemonLog(ERROR, "lost connection to notify_watch");
	config_setup(task, cfg);
    }

    if(cfg->eventFd >= 0){
	task->fds[task->nfds].fd = cfg->eventFd;
	task->fds[task->nfds].events = POLLIN;
	task->nfds++;
    }

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
	lemonLog(ERROR, "Failed to create inofity fd %s - "
			 "hot reload disabled", strerror(errno));
	task->exitStatus = DO_NOTHING;
	return;
    }

    cfg->watchFd = inotify_add_watch(cfg->eventFd, cfg->configPath, 
	    IN_CLOSE_WRITE);

    if (cfg->watchFd == -1) {
	lemonLog(ERROR, "Failed to add watcher inofity fd %s - ", 
			"hot reload disabled", strerror(errno));
	task->exitStatus = DO_NOTHING;
	return;
    }

    task->exitStatus = FINE;
}

private_ void config_handleEvents(
	struct taskRunner *task,
	void *_cfg_)
{
    struct configuration *cfg = _cfg_;
    struct inotify_event event = {0};

    ssize_t len = read(cfg->eventFd, (void*)&event, 
	    sizeof(struct inotify_event));
    
    if (len < 0){
	lemonLog(ERROR, "Failed to read inotify event - read size: %llu"
	    "inotify_event size: %llu\n", len, sizeof(struct inotify_event));
	task->cleanTask = config_closeWatch;
	task->exitStatus = NON_FATAL;
	return;
    }

    if (!(event.mask & IN_CLOSE_WRITE)){
	lemonLog(DEBUG, "Did not recive IN_CLOSE_WRITE - unexpected");
	task->exitStatus = FINE;
	return;
    }

    task->exitStatus = FINE;
}


private_ void config_closeWatch(
	struct taskRunner *task,
	void *_cfg_)
{
    struct configuration *cfg = _cfg_;
    close(cfg->eventFd);
    close(cfg->watchFd);
    cfg->eventFd = -1;
    cfg->watchFd = -1;
}

private_ void config_readConfiguration(
	struct taskRunner *task,
	void *_cfg_)
{
    struct configuration *cfg = _cfg_;
    FILE *fcfg = fopen(cfg->configPath, "r");
    if(fcfg == NULL){
	lemonLog(DEBUG, "Failed to open configuration %s", cfg->configPath);
	task->exitStatus = DO_NOTHING;
	return;
    }

    if(cfg->mcfg.configMap == NULL){
	cfg->mcfg.configMap = hashmap_new();
    }


    char line[256] = {0};
    char section[128] = {0};
    ssize_t sectionLength = 0;
    char outKey[128] = {0};
    char outVal[128] = {0};
    for(int i = 0; i < NCONFIG_PARAM && 
	    NULL != fgets(line, sizeof(line), fcfg); i++){

	if(line[0] == '#' || line[0] == '\n') continue;
	ssize_t lineLength = strlen(line);
	if(line[0] == '[' || line[lineLength] == ']'){
	    memset(section, 0, sizeof(section));
	    strncpy(section, &line[1], lineLength - 3);
	    sectionLength = strlen(section);
	} else {
	    config_parseConfigValue(outKey, outVal, 
		    section, line, lineLength, sectionLength);

	   strcpy(cfg->mcfg.key[i], outKey);
    	   strcpy(cfg->mcfg.value[i], outVal);
    	   hashmap_put(cfg->mcfg.configMap, 
    	   	cfg->mcfg.key[i], (void *)cfg->mcfg.value[i]);	
	   memset(outVal, 0, sizeof(outVal));
	   memset(outKey, 0, sizeof(outKey));
	}
    	memset(line, 0, sizeof(line));
    }

    task->exitStatus = FINE;
    fclose(fcfg);
}

static inline void config_parseConfigValue(
	char *outKey,
	char *outVal,
	char *section, 
	char *line, 
	ssize_t lineLength,
	ssize_t sectionLength)
{
    int i, j, k;
    for(i = 0; i < lineLength; i++){
	if(line[i] == '=') {
	    line[i - 1] = '\0';
	    break;
	}
	if(line[i] == ' '){
	    line[i] = line[i + 1];
	}
    }

    sprintf(outKey, "%.56s_%.56s", section, line);

    for(k = i; k < lineLength; k++){
	if(line[k] == ' ') continue;
	if(line[k] == '=') continue;
	break;
    }

    for(j = k; j < lineLength; j++){
	if(line[j] == '\n') break;
    }
    strncpy(outVal, &line[k], j - k);
}
