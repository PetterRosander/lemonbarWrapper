#define _XOPEN_SOURCE
#define _DEFAULT_SOURCE
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <locale.h>
#include <getopt.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>

#include "task-runner.h"
#include "plugins.h"
#include "configuration-manager.h"
#include "lemonCommunication.h"
#include "workspace.h"
#include "sys-utils.h"

#define DEFAULT_CONFIG "/.config/lemonwrapper/lemon.config"
#define DEFAULT_LOG    "/.local/run/lemon.log"

bool request_exit = false;

bool main_exitRequested(void)
{
    return request_exit;
}

static void signalHandler(int sig)
{
    switch(sig){
	case SIGCHLD:
	    {
		int status = 0;
		pid_t pid = wait(&status);
		(void)pid;
	    }
	    break;
	case SIGINT:
	case SIGTERM:
	    request_exit = true;
	    break;
    }
}

static void __attribute__((noreturn)) printHelp(char *argv[])
{
    printf("Usage: %s [options]\n", argv[0]);
    printf("Options:\n");
    printf("  --help      -h\tprintf this help message\n");
    printf("  --no-daemon -d\truns program without daemonizing\n");
    printf("  --log-path  -l\tspecify a different log path [default:"
	                            " %s]\n", DEFAULT_LOG);
    fflush(stdout);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{

    int opt;
    char i3path[126] = {0};
    char config[128] = {0};
    char logpath[128] = {0};

    pid_t pid = pidof(argv[0]);
    pid_t me = getpid();

    if(pid != me){
        return 0;
    }

    struct taskRunner _task = {0};
    struct taskRunner *task = &_task;

    char *home = getenv("HOME");
    ssize_t lenHome = strlen(home);

    int uid = getuid();
    pid_t i3 = pidof("i3");
    snprintf(i3path, sizeof(i3path), "/var/run/user/%d/i3/ipc-socket.%d", 
	    uid, i3);

    memcpy(logpath, home, lenHome);
    strcat(logpath, DEFAULT_LOG);

    memcpy(config, home, lenHome);
    strcat(config, DEFAULT_CONFIG);


    // TODO: Add long opts (see manual getopt)
    int index = -1;
    struct option long_options[4] = {
	{"help", no_argument, 0, 'h'},
	{"no-daemon", no_argument, 0, 'd'},
	{"log-path", required_argument, 0, 'l'},
	{0, 0, 0, 0}
    };

    task->lp.daemonize = true;
    while((opt = getopt_long(argc, argv, "hdl:", long_options, &index)) != -1){
	switch(opt){
	    case 'l':
		strcpy(logpath, optarg);
		break;
	    case 'd':
		task->lp.daemonize = false;
		break;
	    case 'h':
	    case '?':
	    default:
		printHelp(argv);
		break;
	}
	index = -1;
    }


    if(task->lp.daemonize){
	int i = daemon(0, 0);
	if(i == -1){
	    lemonLog(ERROR, "Failed to daemonize %s", strerror(errno));
	}
	lemonLogInit(&task->lp, logpath);
    }

    lemonLog(DEBUG, "%s started ipc-socket (i3) %s", argv[0], i3path);


    signal(SIGCHLD, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    if(setlocale(LC_ALL, "C.UTF-8") == NULL){
	lemonLog(ERROR, "Failed to set locale to C.UTF-8"
		"exiting due to the error %s", strerror(errno));
	exit(EXIT_FAILURE);
    }

    struct configuration *cfg = config_init(config);
    if(cfg == NULL){
	lemonLog(ERROR, "Failed create configuration %s", strerror(errno));
    }
    cfg->setup(task, cfg);
    
    struct workspace *ws = workspace_init(i3path, cfg);
    if(ws == NULL){
	lemonLog(ERROR, "Failed create workspace %s", strerror(errno));
    }
    ws->setup(task, ws);


    struct plugins *pl = plug_init(cfg);
    if(pl == NULL){
	lemonLog(ERROR, "Failed create plugin %s", strerror(errno));
    }
    pl->setup(task, pl);
    pl->normal(task, pl);

    struct lemonbar *lm = lemon_init(cfg);
    if(pl == NULL){
	lemonLog(ERROR, "Failed create plugin %s", strerror(errno));
    }
    lm->ws = ws;
    lm->pl = pl;
    lm->setup(task, lm);

    if(task && ws && pl && cfg && lm){
	runLoop(task, ws, pl, cfg, lm);
    } else {
	lemonLog(ERROR, "Exiting due to unrecoverable error");
    }

    // free on NULL is noop    
    workspace_destroy(ws);
    lemon_destroy(lm);
    config_destory(cfg);
    plug_destroy(pl);



    return 0;
}
