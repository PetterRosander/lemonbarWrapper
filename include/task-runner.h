#ifndef _TASKRUNNER_H_
#define _TASKRUNNER_H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "lemonCommunication.h"
#include "workspace.h"
#include "configuration-manager.h"
#include "plugins.h"

struct taskRunner;
typedef void (*next)(struct taskRunner *, void *);

struct taskRunner {
    next nextTask;
    void *arg;
    int exitStatus;
    char function[100];
};

int taskRunner_runTask(struct taskRunner);
void runLoop(
	struct workspace *, 
	struct plugins *,
	struct configuration *cfg,
	struct lemonbar *);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _TASKRUNNER_H_ */
