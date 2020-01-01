#ifndef _SYS_UTILS_H_
#define _SYS_UTILS_H_
#include <sys/types.h>

pid_t pidof(char *);
int pkill(char *);
pid_t popen2(
	const char *,
	int *,
	int *);

#endif
