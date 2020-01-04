/******************************************************************************
 * PreProcessor directive
 *****************************************************************************/
#define __PLUGINS__
#include "plugins.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include "configuration-manager.h"
#include "task-runner.h"
#include "hashmap.h"
/******************************************************************************
 * exported functions declaration
 *****************************************************************************/
/* 
 * Setup function used to access the 
 * functionallity in this modue
 */
struct plugins *plug_init(struct configuration *cfg)
{
    struct plugins *pl = calloc(1, sizeof(struct plugins));
    if(pl == NULL){
	return NULL;
    }
    
    char *value = NULL;
    hashmap_get(cfg->mcfg.configMap, "WIFI_SYMBOL",
	    (void**)(&value));
    pl->plcfg._wifiSymbol = value;

    hashmap_get(cfg->mcfg.configMap, "BATTERY_NORMAL",
	    (void**)(&value));
    pl->plcfg._batteryNormal = value;

    hashmap_get(cfg->mcfg.configMap, "BATTERY_CHARGING",
	    (void**)(&value));
    pl->plcfg._batteryCharging = value;

    hashmap_get(cfg->mcfg.configMap, "LOW_BATTERY_WARNING",
	    (void**)(&value));
    pl->plcfg._warnPercent = value;

    pl->setup = plug_setup;
    pl->reconfigure = plug_reconfigure;
    pl->normal = plug_runNormal;
    pl->event = plug_runEvent;
    return pl;

}

/*
 * Clear memory assiciated with the
 * confugration struct
 */
int plug_destroy(struct plugins *cfg)
{
    free(cfg);
    cfg = NULL;
    return 0;
}

/******************************************************************************
 * lemonCommunication entry functions
 *****************************************************************************/
private_ void plug_setup(
	struct taskRunner *task,
	struct plugins *pl)
{
    task->nextTask[0] = plug_setupPipe;
    task->nextTask[1] = plug_configure;
    task->nbrTasks = 2;
    task->arg = pl;
    taskRunner_runTask(task);
}

private_ void plug_reconfigure(
	struct taskRunner *task,
	struct plugins *pl)
{
    task->nextTask[0] = plug_configure;
    task->nbrTasks = 1;
    task->arg = pl;
    taskRunner_runTask(task);
}

private_ void plug_runNormal(
	struct taskRunner *task,
	struct plugins *pl)
{
    task->nextTask[0] = plug_getBattery;
    task->nextTask[1] = plug_getWifi;
    task->nextTask[2] = plug_getTime;
    task->nbrTasks = 3;
    task->arg = pl;
    taskRunner_runTask(task);
}

private_ void plug_runEvent(
	struct taskRunner *task,
	struct plugins *pl)
{
    task->nextTask[0] = plug_lockOrShutdown;
    task->nbrTasks = 1;
    task->arg = pl;
    taskRunner_runTask(task);
}

/******************************************************************************
 * Local function declarations
 *****************************************************************************/
private_ void plug_lockOrShutdown(
	struct taskRunner *task,
	void *_pl_)
{
    struct plugins *pl = _pl_;
    char packet[2] = {0};
    int readBytes = read(pl-> pluginsFd, packet, 2);
    (void) readBytes;

    if(strncmp(packet, "LS", 2) == 0){
	lemonLog(DEBUG, "reuqested lock-logout-shutdown");
	pl->shutdownOrLock = true;
    }

    task->exitStatus = 0;
}

private_ void plug_configure(
	struct taskRunner *task,
	void *_pl_)
{
    struct plugins *pl = _pl_;
    pl->plcfg.wifiSymbol = (wchar_t)strtol(pl->plcfg._wifiSymbol, NULL, 16);
    pl->plcfg.batteryNormal = (wchar_t)strtol(pl->plcfg._batteryNormal, NULL, 16);
    pl->plcfg.batteryCharging = (wchar_t)strtol(pl->plcfg._batteryCharging, NULL, 16);

    char *value = pl->plcfg._warnPercent;
    if(value != NULL){
	value = strtok(value, ",");

	pl->plcfg.warnPercent[0] = strtol(value, NULL, 10);
	pl->plcfg.notifyWarn[0] = true;
	for(int i = 1; i < MAX_BATTERY_WARN && NULL != (value = strtok(NULL, ",")); i++){
	    pl->plcfg.warnPercent[i] = strtol(value, NULL, 10);
	    pl->plcfg.notifyWarn[i] = true;
	}
    }
    task->exitStatus = 0;

}

private_ void plug_setupPipe(
	struct taskRunner *task,
	void *_pl_)
{
    struct plugins *pl = _pl_;
    char *home = getenv("HOME");
    char readPipe[100] = {0};
    strcpy(readPipe, home);
    strcat(readPipe, "/.local/run/lemonwrapper.pipe");

    struct stat st = {0};
    if(stat(readPipe, &st) != 0){
	if(mkfifo(readPipe, 0666) == -1){
	    lemonLog(ERROR, "Failed to create pipe %s", strerror(errno));
	    task->exitStatus = -1;
	    return;
	}
    } else {
	lemonLog(DEBUG, "Pipe already exists");
    }
    // Since the pipe will be opened and closed multiple times
    // We do not want poll to hand on POLLHUP
    pl->pluginsFd = open(readPipe, O_RDWR | O_NONBLOCK);
    if(pl->pluginsFd < 0){
	lemonLog(ERROR, "failed to open pipe %s", strerror(errno));
	task->exitStatus = -1;
	return;
    }

    task->exitStatus = 0;
}

private_ void plug_startUserPlugins(
	struct taskRunner *task,
	void *_pl_)
{
    task->exitStatus = 0;
}

#define FBATTERY_CAPACITY "/sys/class/power_supply/BAT0/capacity"
#define FBATTERY_STATUS   "/sys/class/power_supply/BAT0/status"
private_ void plug_getBattery(
	struct taskRunner *task,
	void *_pl_)
{
    struct plugins *pl = _pl_;
    FILE *fBat = fopen(FBATTERY_CAPACITY, "r");

    int i = fscanf(fBat, "%hhu", &pl->bat.capacity);
    (void)i;

    fclose(fBat);


    fBat = fopen(FBATTERY_STATUS, "r");
    char tmpStatus[512] = {0};
    i = fscanf(fBat, "%s", tmpStatus);
    fclose(fBat);
    
    pl->pluginsLen = 0;
    memset(pl->pluginsFormatted, 0, sizeof(pl->pluginsFormatted));

    char colorShown[50] = {0};

    if(pl->bat.capacity > 66){
	strcpy(colorShown, "0F0");
    } else if(pl->bat.capacity > 33){
	strcpy(colorShown, "FF0");
    } else {
	strcpy(colorShown, "F00");
    }


    if(strcmp("Discharging", tmpStatus) == 0){
        pl->pluginsLen += sprintf(&pl->pluginsFormatted[pl->pluginsLen], 
    	    "%%{+u}%%{U#%s}%u%% %lc%%{-u}%%{U-} | ", colorShown , 
	    pl->bat.capacity, pl->plcfg.batteryNormal);
    } else if (strcmp("Full", tmpStatus) == 0){
        pl->pluginsLen += sprintf(&pl->pluginsFormatted[pl->pluginsLen], 
    	    "%%{+u}%%{U#0F0}%u%% %lc%%{-u}%%{U-} | ", 
	    pl->bat.capacity, pl->plcfg.batteryNormal);
    } else if (strcmp("Charging", tmpStatus) == 0){
        pl->pluginsLen += sprintf(&pl->pluginsFormatted[pl->pluginsLen], 
    	    "%%{+u}%%{U#0F0}%u%% %lc%%{-u}%%{U-} | ", 
	    pl->bat.capacity, pl->plcfg.batteryCharging);
    }

    for(int i = 0; i < MAX_BATTERY_WARN; i++){
	if(pl->bat.capacity <= pl->plcfg.warnPercent[i] && 
		pl->plcfg.notifyWarn[i]){
	    
	    int i = system("notify-send Battery \"low battery\"");
	    if(i < 0){
		lemonLog(ERROR, "sending notificatication faild %s", 
			strerror(errno));
	    }
	    pl->plcfg.notifyWarn[i] = false;

	} else if(pl->bat.capacity > pl->plcfg.warnPercent[i]) {
	    pl->plcfg.notifyWarn[i] = true;
	}
    }

    task->exitStatus = 0;
}

#define FWIFI_LINK "/proc/net/wireless"
private_ void plug_getWifi(
	struct taskRunner *task,
	void *_pl_)
{
    struct plugins *pl = _pl_;
    FILE *fWifi = fopen(FWIFI_LINK, "r");
    char ignore[512];

    char *ptr = fgets(ignore, sizeof(ignore), fWifi);
    ptr = fgets(ignore, sizeof(ignore), fWifi);
    (void)ptr;

    int i = fscanf(fWifi, "%s %s %hhu %s", ignore, ignore, &pl->wf.link, ignore);

    if(i != 4){
	pl->wf.link = 0;
    }
    


    if(pl->wf.link > 50){
        pl->pluginsLen += sprintf(&pl->pluginsFormatted[pl->pluginsLen], 
    	    "%%{+u}%%{U#0F0}%lc%%{U-}%%{-u} | ", pl->plcfg.wifiSymbol);
    } else if(pl->wf.link > 20){
        pl->pluginsLen += sprintf(&pl->pluginsFormatted[pl->pluginsLen], 
    	    "%%{+u}%%{U#FF0}%lc%%{U-}%%{-u} | ", pl->plcfg.wifiSymbol);
    } else {
        pl->pluginsLen += sprintf(&pl->pluginsFormatted[pl->pluginsLen], 
    	    "%%{+u}%%{U#F00}%lc%%{U-}%%{-u} | ", pl->plcfg.wifiSymbol);
    }

    fclose(fWifi);
    task->exitStatus = 0;
}

private_ void plug_getTime(
	struct taskRunner *task,
	void *_pl_)
{
    struct plugins *pl = _pl_;
    pl->td.time = time(NULL);
    
    struct tm *tm_info = NULL;
    tm_info = localtime(&pl->td.time);

    strftime(pl->td.bufTime, 6, "%H:%M", tm_info);
    pl->pluginsLen += sprintf(&pl->pluginsFormatted[pl->pluginsLen], 
	    "%s ", pl->td.bufTime);
    task->exitStatus = 0;
}

