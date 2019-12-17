#define _XOPEN_SOURCE
#include <unistd.h>
#include <string.h>

#include "lemonCommunication.h"
#include "workspace.h"
#include "task-runner.h"

#define ERROR(error) \
    fprintf(stderr, "%s\n", error); \
    exit(EXIT_FAILURE);


int main(int argc, char *argv[])
{

    int opt;
    char i3path[128] = {0};

    // TODO: Add long opts (see manual getopt)
    while((opt = getopt(argc, argv, "p:")) != -1) {
	switch(opt){
	case 'p':
	    strcpy(i3path, optarg);
	    printf("%s\n", i3path);
	    break;
	default:
	    printf("Unknown option"); 
	    break;
	}
    }

    struct workspace *ws = workspace_init(i3path);
    ws->setup(ws);

    struct lemonbar *lm = lemon_init();
    lm->setup(lm);

    runLoop(ws, lm);

    return 0;
}
