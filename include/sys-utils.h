#ifndef _SYS_UTILS_H_
#define _SYS_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <sys/types.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>


pid_t pidof(char *);
int pkill(char *);
pid_t popen2(
	const char *,
	int *,
	int *);


#define MAX_ROWS 128
enum LEMON_LOGLEVEL {
    DEBUG,
    ERROR
};

struct log_t {
    FILE *fp;
    bool daemonize;
    char filepath[120];
    char logfile[124];
    uint8_t logfilenum;
    uint8_t nRows;
    const char *func;
    uint32_t line;
};


void _lemonLog(struct log_t *, enum LEMON_LOGLEVEL, char *format, ...);
void lemonLogInit(struct log_t *lp, const char *path);
void lemonLogClose(struct log_t *lp);

#define lemonLog(level, ...) \
    task->lp.line = __LINE__; \
    task->lp.func = __func__; \
    _lemonLog(&task->lp, level, __VA_ARGS__);

#ifdef __cplusplus
}
#endif

#endif
