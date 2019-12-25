/******************************************************************************
 * PreProcessor directive
 *****************************************************************************/
#define __PLUGINS__
#include "plugins.h"
#include "configuration-manager.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

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
    pl->plcfg.wifiSymbol = (wchar_t)strtol(value, NULL, 16);

    hashmap_get(cfg->mcfg.configMap, "BATTERY_NORMAL",
	    (void**)(&value));
    pl->plcfg.batteryNormal = (wchar_t)strtol(value, NULL, 16);

    hashmap_get(cfg->mcfg.configMap, "BATTERY_CHARGING",
	    (void**)(&value));
    pl->plcfg.batteryCharging = (wchar_t)strtol(value, NULL, 16);

    hashmap_get(cfg->mcfg.configMap, "LOW_BATTERY_WARNING",
	    (void**)(&value));


    if(value != NULL){
	value = strtok(value, ",");

	pl->plcfg.warnPercent[0] = strtol(value, NULL, 10);
	pl->plcfg.notifyWarn[0] = true;
	for(int i = 1; i < MAX_BATTERY_WARN && NULL != (value = strtok(NULL, ",")); i++){
	    pl->plcfg.warnPercent[i] = strtol(value, NULL, 10);
	    pl->plcfg.notifyWarn[i] = true;
	}
    }

    pl->setup = plug_setup;
    pl->normal = plug_runNormal;
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
private_ void plug_setup(struct plugins *pl)
{
    struct taskRunner task = {0};
    task.nextTask = plug_startUserPlugins;
    task.arg = pl;
    taskRunner_runTask(task);
}

private_ void plug_runNormal(struct plugins *pl)
{
    struct taskRunner task = {0};
    task.nextTask = plug_getBattery;
    task.arg = pl;
    taskRunner_runTask(task);
}

/******************************************************************************
 * Local function declarations
 *****************************************************************************/
private_ void plug_startUserPlugins(
	struct taskRunner *task,
	void *_pl_)
{
    task->nextTask = NULL;
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
    char tmpStatus[100] = {0};
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
	if(pl->bat.capacity < pl->plcfg.warnPercent[i] && 
	   pl->plcfg.notifyWarn[i]){
	    task->exitStatus = 0;
	    task->nextTask = plug_notifyBattery;
	    pl->plcfg.notifyWarn[i] = false;
	} else if(pl->bat.capacity > pl->plcfg.warnPercent[i]) {
	    pl->plcfg.notifyWarn[i] = true;
	}
    }

    if(task->nextTask == plug_getBattery){
	task->exitStatus = 0;
    	task->nextTask = plug_getWifi;
    }
}

/*
 * There is a lib for this but 
 * want to limit dependencies
 */
private_ void plug_notifyBattery(
	struct taskRunner *task,
	void *_pl_)
{
    int i = system("notify-send Battery \"low battery\"");
    (void)i;
    task->exitStatus = 0;
    task->nextTask = plug_getWifi;
}

#define FWIFI_LINK "/proc/net/wireless"
private_ void plug_getWifi(
	struct taskRunner *task,
	void *_pl_)
{
    struct plugins *pl = _pl_;
    FILE *fWifi = fopen(FWIFI_LINK, "r");
    char ignore[128];
    char *ptr = fgets(ignore, sizeof(ignore), fWifi);
    ptr = fgets(ignore, sizeof(ignore), fWifi);
    int i = fscanf(fWifi, "%s %s %hhu %s", ignore, ignore, &pl->wf.link, ignore);
    
    (void)i;
    (void)ptr;

    task->exitStatus = 0;
    task->nextTask = plug_getTime;

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
    task->nextTask = NULL;
}

