#include <unistd.h> // pause

#include "commChannel.h"
#include "i3query.h"

#define ERROR(error) \
    fprintf(stderr, "%s\n", error);


int main(int argc, char *argv[])
{
    struct lemonbar lm = {0};
    if(initChannel(&lm) != 0){
	perror("setting up bar");
    }

    if(formatLemonBar(&lm, "hello hello") != 0){
	perror("setting up bar");
    }

    if(sendlemonBar(lm)!= 0){
	perror("send error ");
    }
    //pause();
    struct workspace ws;
    if(subscribeWorkSpace(&ws, "/run/user/1000/i3/ipc-socket.1277") != 0){
	perror("subscribe error");
    }
    
    if(startWorkSpace(&ws) != 0){
	perror("subscribe error");
    }

    if(eventi3(&ws) != 0){
	perror("subscribe error");
    }
    return 0;
}
