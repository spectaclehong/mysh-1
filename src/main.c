#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include "commands.h"
#include "built_in.h"
#include "utils.h"
#include "signal_handlers.h"
int main(){ 
  putenv("PATH=/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin");
  signal_setting();
  char buf[8096]={0}; 
  while (1) {
    memset(buf,0,sizeof buf);
    fgets(buf, 8096, stdin);
     
    struct single_command commands[512];
    int n_commands = 0;

    int background = 0;
    int buflen = strlen(buf);
    for(int i = 0;i < buflen; i++) if(buf[i] == '&'){
      background = 1;
      buf[i] = 0;
      memset(running_command,0,1024);
      strcpy(running_command,buf);
      break;
    }
    mysh_parse_command(buf, &n_commands, &commands);
    
    if(background){
      PID = fork();
      if(PID) {
        fprintf(stdout,"%d\n",PID);
        free_commands(n_commands,&commands);
        n_commands = 0;
        continue;
      }
    } 
   
    int ret = evaluate_command(n_commands, &commands);
    free_commands(n_commands, &commands);
    n_commands = 0;
    
    if(PID == 0){
      if(ret == 0) 
        fprintf(stdout,"%d done %s\n",getpid(),running_command);
      free_commands(n_commands, &commands);
      n_commands = 0; 
      exit(0);
    }
    free_commands(n_commands, &commands);
    n_commands = 0;
    
    if (ret == 1) {
      break;
    }
  }
  return 0;
}
