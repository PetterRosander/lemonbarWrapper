#define _XOPEN_SOURCE
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <locale.h>

#include "task-runner.h"
#include "plugins.h"
#include "configuration-manager.h"
#include "lemonCommunication.h"
#include "workspace.h"
#include "sys-utils.h"

#define DEFAULT_CONFIG "/.config/lemonwrapper/lemon.config"

bool request_exit = false;

bool main_exitRequested(void)
{
    return request_exit;
}

static void signalHandler(int sig)
{
    switch(sig){
	case SIGCHLD:
	    break;
	case SIGINT:
	case SIGTERM:
	    request_exit = true;
	    break;
    }
}

int main(int argc, char *argv[])
{

    int opt;
    char i3path[126] = {0};
    char config[128] = {0};

    pid_t pid = pidof(argv[0]);
    pid_t me = getpid();

    if(pid != me){
	return 0;
    }
    

    char *home = getenv("HOME");
    char *_i3path = getenv("I3SOCK");
    strcpy(i3path, _i3path);

    memcpy(config, home, strlen(home));
    strcat(config, DEFAULT_CONFIG);

    // TODO: Add long opts (see manual getopt)
    while((opt = getopt(argc, argv, "h")) != -1) {
	switch(opt){
	case 'h':
	    printf("TODO: help\n");
	    break;
	default:
	    printf("Unknown option"); 
	    break;
	}
    }


    signal(SIGCHLD, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    if(setlocale(LC_ALL, "C.UTF-8") == NULL){
        printf("Falied to set locale\n");
        return 198;
    }

    struct configuration * cfg = config_init(config);
    cfg->setup(cfg);
    
    struct workspace *ws = workspace_init(i3path);
    ws->setup(ws);


    struct plugins *pl = plug_init(cfg);
    pl->setup(pl);
    pl->normal(pl);

    struct lemonbar *lm = lemon_init(cfg);
    lm->ws = ws;
    lm->pl = pl;
    lm->setup(lm);

    runLoop(ws, pl, cfg, lm);
    
    workspace_destroy(ws);
    lemon_destroy(lm);
    config_destory(cfg);
    plug_destroy(pl);


    return 0;
}
