#define _POSIX_SOURCE
#define _GNU_SOURCE
#include <time.h>
#include <stdarg.h>
#include <stdint.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>

#include "sys-utils.h"
/*
 * Reads proc list and return the first match
 * of a program (does not need to be more advanced for my use)
 */
pid_t pidof(char *_cmdline)
{

    DIR *dir = opendir("/proc/");
    struct dirent *entry = NULL;
    char filepath[512] = {0};
    char cmdline[512*5] = {0};
    FILE *fp = NULL;
    bool found = false;
    char *ptr;
    (void)ptr;

    while((entry = readdir(dir)) != NULL){
	if(!isdigit(entry->d_name[0])) continue;
	snprintf(filepath, sizeof(filepath), 
		"/proc/%s/cmdline", entry->d_name);

	fp = fopen(filepath, "r");
	if(fp == NULL) continue;

	size_t size = 0;
	size_t cSize = 0;
	char *arg = NULL;
	while(getdelim(&arg, &size, 0, fp) != -1){
	    cSize += sprintf(&cmdline[cSize], "%s ", arg);
	}
	free(arg);
	fclose(fp);

	if(strncmp(cmdline, _cmdline, strlen(_cmdline)) == 0){
	    found = true;
	    break;
	}

    }

    pid_t ret = 0;

    if(found){
	ret = strtol(entry->d_name, NULL, 10);
    }


    closedir(dir);
    return ret;

}

int pkill(char *cmdline)
{
    pid_t pid = pidof(cmdline);
    if(pid == 0){
	return -1;
    }
    return kill(pid, SIGTERM);
}


#define PIPE_READ 0
#define PIPE_WRITE 1
/*
 * Since popen does not support bidirectional
 * communication we write our own popen 
 */
pid_t popen2(
	const char *command,
	int *infp,
	int *outfp)
{
    int p_stdin[2], p_stdout[2];

    pid_t pid;

    if (pipe(p_stdin) != 0 || pipe(p_stdout) != 0){
	return -1;
    }

    pid = fork();

    if (pid < 0){
	return pid;
    } else if (pid == 0){
	close(p_stdin[PIPE_WRITE]);
	dup2(p_stdin[PIPE_READ], PIPE_READ);
	close(p_stdout[PIPE_READ]);
	dup2(p_stdout[PIPE_WRITE], PIPE_WRITE);
	execl("/bin/sh", "sh", "-c", command, NULL);
	perror("execl");
	exit(1);
    }

    if (infp == NULL){
	close(p_stdin[PIPE_WRITE]);
    } else{
	*infp = p_stdin[PIPE_WRITE];
    }

    if (outfp == NULL){
	close(p_stdout[PIPE_READ]);
    } else {
	*outfp = p_stdout[PIPE_READ];
    }

    return pid;
}


void lemonLogInit(struct log_t *lp, const char *path)
{
    strcpy(lp->filepath, path);
    sprintf(lp->logfile, "%s.%i", lp->filepath, lp->logfilenum);
    lp->logfilenum += 1;
    if(lp->logfilenum > 4){
	lp->logfilenum = 0;
    }
    lp->fp = fopen(lp->logfile, "w");
}

void _lemonLog(struct log_t *lp, enum LEMON_LOGLEVEL level, char *format, ...)
{
    va_list list;

    char logrow[512*4] = {0};
    va_start(list, format);
    vsnprintf(logrow, sizeof(logrow), format, list);
    va_end(list);

    if(lp->nRows > MAX_ROWS){
	lp->logfilenum += 1;
    	if(lp->logfilenum > 4){
    	    lp->logfilenum = 0;
    	}
	sprintf(lp->logfile, "%s.%i", lp->filepath, lp->logfilenum);
	fclose(lp->fp);
	lp->fp = fopen(lp->logfile, "w");
    }

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    char asciiTime[9] = {0};
    strftime(asciiTime, 9, "%H:%M:%S", tm_info);

    if(lp->daemonize){
	fprintf(lp->fp, "[%s - %s]: %s [%s - %d]\n", asciiTime, 
		level ? "ERROR" : "DEBUG", logrow, lp->func, lp->line);
	fflush(lp->fp);
    } else {
	FILE *out = NULL;
	switch(level){
	    case ERROR:
		out = stderr;
		break;
	    case DEBUG:
		out = stdout;
		break;
	}
	fprintf(out, "[%s - %s]: %s [%s - %d]\n", asciiTime, 
		level ? "ERROR" : "DEBUG", logrow, lp->func, lp->line);
    }
}

void lemonLogClose(struct log_t *lp)
{
    fclose(lp->fp);
}
