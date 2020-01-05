#ifndef _PLUGINS_H_
#define _PLUGINS_H_

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************
 * PreProcessor directives
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "task-runner.h"
#include "configuration-manager.h"
#define MAX_BATTERY_WARN 4

/******************************************************************************
 * Declarations
 *****************************************************************************/
struct plugins;
typedef void (*_plugins_entryPoint_)(struct taskRunner *, struct plugins *);

/******************************************************************************
 * Structs
 *****************************************************************************/
struct battery {
    uint8_t capacity;
};

struct wifi {
    uint8_t link;
};

struct timeofday {
    time_t time;
    char   bufTime[6];
};

struct pluginConfig {
    wchar_t wifiSymbol;
    wchar_t batteryNormal;
    wchar_t batteryCharging;

    char *_wifiSymbol;
    char *_batteryNormal;
    char *_batteryCharging;
    char *_warnPercent;

    bool notifyWarn[MAX_BATTERY_WARN];
    uint8_t warnPercent[MAX_BATTERY_WARN];
};

struct plugins {
    struct battery bat;
    struct wifi wf;
    struct timeofday td;

    char pluginsFormatted[160];
    uint8_t pluginsLen;
    struct pluginConfig plcfg;

    int pluginsFd;
    bool shutdownOrLock;

    _plugins_entryPoint_ setup;
    _plugins_entryPoint_ addFd;
    _plugins_entryPoint_ reconfigure;
    _plugins_entryPoint_ event;
    _plugins_entryPoint_ normal;
};

/******************************************************************************
 * Exported function
 *****************************************************************************/
struct plugins *plug_init(struct configuration *);
int plug_destroy(struct plugins*);

/******************************************************************************
 * Local function declarations
 *****************************************************************************/
#if UNIT_TEST
#define private_
#else
#define private_ static
#endif

#ifdef __PLUGINS__
#include "task-runner.h"
private_ void plug_setup(
	struct taskRunner *,
	struct plugins *);
private_ void plug_reconfigure(
	struct taskRunner *,
	struct plugins *);
private_ void plug_runNormal(
	struct taskRunner *,
	struct plugins *);
private_ void plug_runEvent(
	struct taskRunner *,
	struct plugins *);
private_ void plug_addFd(
	struct taskRunner *,
	struct plugins *);

private_ void plug_lockOrShutdown(
	struct taskRunner *,
	void *);
private_ void plug_setupPipe(
	struct taskRunner *,
	void *);
private_ void plug_configure(
	struct taskRunner *,
	void *);
private_ void plug_startUserPlugins(
	struct taskRunner *,
	void *);
private_ void plug_getBattery(
	struct taskRunner *,
	void *);
private_ void plug_notifyBattery(
	struct taskRunner *,
	void *);
private_ void plug_getWifi(
	struct taskRunner *,
	void *);
private_ void plug_getTime(
	struct taskRunner *,
	void *);

#endif /* __PLUGINS__ */
#undef __PLUGINS__

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _CONFIGURATION_MANGER_H_ */
