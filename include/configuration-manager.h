#ifndef _CONFIGURATION_MANGER_H_
#define _CONFIGURATION_MANGER_H_

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************
 * PreProcessor directives
 *****************************************************************************/
#include <wchar.h>

#include "hashmap.h"
#include "task-runner.h"
#define NCONFIG_PARAM 12

/******************************************************************************
 * Declarations
 *****************************************************************************/
struct configuration;
typedef void (*_config_entryPoint_)(struct taskRunner *, struct configuration *);

/******************************************************************************
 * Structs
 *****************************************************************************/


struct moduleConfig {
    map_t configMap;
    char key[NCONFIG_PARAM][56];
    char value[NCONFIG_PARAM][56];
};

struct configuration {
    int eventFd;
    int watchFd;
    char configPath[100];
    struct moduleConfig mcfg;

    _config_entryPoint_ addFd;
    _config_entryPoint_ setup;
    _config_entryPoint_ event;
};

/******************************************************************************
 * Exported function
 *****************************************************************************/
struct configuration *config_init(const char*path);
int config_destory(struct configuration*);

#if UNIT_TEST
#define private_
#else
#define private_ static
#endif

#ifdef __CONFIGURATION__
#include "task-runner.h"
private_ void config_setup(
	struct taskRunner *,
	struct configuration *);
private_ void config_event(
	struct taskRunner *,
	struct configuration *);
private_ void config_addFd(
	struct taskRunner *,
	struct configuration *);

private_ void config_handleEvents(
	struct taskRunner *,
	void *);
private_ void config_addWatcher(
	struct taskRunner *,
	void *);
private_ void config_closeWatch(
	struct taskRunner *,
	void *);
private_ void config_readConfiguration(
	struct taskRunner *,
	void *);

#endif /*__CONFIGURATION__ */
#undef __CONFIGURATION__

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _CONFIGURATION_MANGER_H_ */
