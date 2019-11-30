#include <unistd.h> // pause

#include "commChannel.h"

#define ERROR(error) \
    fprintf(stderr, "%s\n", error);


int main(int argc, char *argv[])
{
    struct lemonbar lm = initChannel();

    if(formatLemonBar(&lm, "hello hello") != 0){
	ERROR("error occured setting up bar");
    }

    if(sendlemonBar(lm)!= 0){
	ERROR("error occured sending bar");
    }
    pause();


    return 0;
}
