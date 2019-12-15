#ifndef _TASKRUNNER_H_
#define _TASKRUNNER_H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct taskRunner;
typedef void (*next)(struct taskRunner *, void *);

struct taskRunner {
    next nextTask;
    void *arg;
    int exitStatus;
    char function[100];
};

int taskRunner_runTask(struct taskRunner task);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _TASKRUNNER_H_ */
