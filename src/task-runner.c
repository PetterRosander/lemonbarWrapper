#include <stdlib.h>

#include "task-runner.h"

int taskRunner_runTask(struct taskRunner task)
{
   while(task.nextTask != NULL){
       task.nextTask(&task, task.arg);
   }
   return task.exitStatus;
}
