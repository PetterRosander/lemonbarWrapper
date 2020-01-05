#ifndef _TASKRUNNER_H_
#define _TASKRUNNER_H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "sys-utils.h"
#define MAX_TASK 10

enum ERRORTYPE {
    DO_NOTHING = -1,
    NON_FATAL = -2,
    CRITICAL = -3,
    FATAL = -4
};


struct taskRunner;
typedef void (*next[MAX_TASK])(struct taskRunner *, void *);
typedef void (*clean)(struct taskRunner *, void *);

struct taskRunner {
    struct log_t lp;

    next nextTask;
    clean cleanTask;
    void *arg;
    uint8_t nbrTasks;
    int exitStatus;
};

int taskRunner_runTask(struct taskRunner *);

/*
 * TODO: should probably include these files
 * but I get some wierd complier errors so for now it
 * is enough for the compiler to know that they
 * are declared somewhere 
 */
struct taskRunner;
struct workspace;
struct plugins;
struct configuration;
struct lemonbar;

void runLoop(
	struct taskRunner *,
	struct workspace *, 
	struct plugins *,
	struct configuration *,
	struct lemonbar *);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _TASKRUNNER_H_ */
