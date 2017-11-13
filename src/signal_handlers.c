#define _GNU_SOURCE
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "built_in.h"
#include "signal_handlers.h"
#include "commands.h"
struct sigaction actC;
struct sigaction actZ;
int MPID; 
void catch_sigint(int signalNo)
{
  // TODO: File this!
  int cur=getpid();
//  printf("%d %d %d %d\n",cur,MPID,PID,running-1);
  if (cur!=MPID && cur!=PID){
    exit(1); 
  }
  fprintf(stderr,"\nYour input Ctrl+C\n");
}
void catch_sigtstp(int signalNo)
{
  // TODO: File this!
  fprintf(stderr,"\nYour input Ctrl+Z\n");
  
}

void signal_setting(){
  MPID=getpid();

  actC.sa_handler = catch_sigint;
  sigemptyset(&actZ.sa_mask);
  sigaction(SIGINT,&actC,NULL);
  
  actZ.sa_handler = catch_sigtstp;  
  sigemptyset(&actZ.sa_mask);
  sigaction(SIGTSTP,&actZ,NULL);
 
}
