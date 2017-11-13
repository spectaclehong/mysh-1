#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <pthread.h>

#include "commands.h"
#include "built_in.h"

#define SOCK_PATH "Socket"

int running;
static struct built_in_command built_in_commands[] = {
  { "cd", do_cd, validate_cd_argv },
  { "pwd", do_pwd, validate_pwd_argv },
  { "fg", do_fg, validate_fg_argv }
};

static void exec(struct single_command* com){
  execv(com->argv[0],com->argv);
  char PATH[200]; 
  char* saveptr=NULL; char* tok;
  sprintf(PATH,"%s",getenv("PATH"));
  tok=strtok_r(PATH,":",&saveptr);
  while(tok){
    char path[200];
    sprintf(path,"%s/%s",tok,com->argv[0]);
    execv(path,com->argv);
    tok=strtok_r(NULL,":",&saveptr);
  }
}

static int is_built_in_command(const char* command_name)
{
  static const int n_built_in_commands = sizeof(built_in_commands) / sizeof(built_in_commands[0]);

  for (int i = 0; i < n_built_in_commands; ++i) {
    if (strcmp(command_name, built_in_commands[i].command_name) == 0) {
      return i;
    }
  }

  return -1; // Not found
}

/*
 * Description: Currently this function only handles single built_in commands. You should modify this structure to launch process and offer pipeline functionality.
 */
void *server_thread(void * n);
void *client_thread(void * com);

int evaluate_command(int n_commands, struct single_command (*commands)[512])
{
  if (n_commands > 0) {
    struct single_command* com = (*commands);

    assert(com->argc != 0);

    int built_in_pos = is_built_in_command(com->argv[0]);
    if (built_in_pos != -1) {
      if (built_in_commands[built_in_pos].command_validate(com->argc, com->argv)) {
        if (built_in_commands[built_in_pos].command_do(com->argc, com->argv) != 0) {
          fprintf(stderr, "%s: Error occurs\n", com->argv[0]);
        }
      } else {
        fprintf(stderr, "%s: Invalid arguments\n", com->argv[0]);
        return -1;
      }
    } else if (strcmp(com->argv[0], "") == 0) {
      return 0;
    } else if (strcmp(com->argv[0], "exit") == 0) {
      return 1;
    } else if(n_commands==1){
      int status=0;
      int pid=getpid();
      running=fork();
      if(running==0){
        exec(com);
        fprintf(stderr, "%s: command not found\n", com->argv[0]);
        exit(1);
      } else{
        waitpid(running,&status,0);
        return status?-1:0;
      }
    } else{ 
      void* status;
      pthread_t threads[2];
      int pid=fork();
      if(pid==0){
        for(int p=0;p<n_commands;p++,com++){ 
          pthread_create(&threads[1],NULL,&client_thread,(void *) com);
          pthread_join(threads[1],&status); 
        }
        exit(0);
      } else{
         pthread_create(&threads[0],NULL,&server_thread,(void *)&n_commands);
         waitpid(pid,NULL,0);
         pthread_join(threads[0],NULL);
      }
    }
  }
  return 0;
}

void *server_thread(void * n){
  int serverSocketFd = 0;
  int clientSocketFd = 0;
  int serverAddrSize = 0;
  socklen_t clientAddrSize = 0;

  struct sockaddr_un serverAddress;
  struct sockaddr_un clientAddress;

  remove(SOCK_PATH);
  serverSocketFd = socket(PF_LOCAL, SOCK_STREAM, 0);

  memset(&serverAddress, 0, sizeof(serverAddress));
  serverAddress.sun_family = AF_LOCAL;
  strcpy(serverAddress.sun_path, SOCK_PATH);
  serverAddrSize = sizeof(serverAddress);
  bind(serverSocketFd, (struct sockaddr *)&serverAddress, serverAddrSize);

  listen(serverSocketFd,5);
  int stdfd=dup(1);
  char buf[1024];
  memset(buf,0,sizeof buf);
  memset(&clientAddress, 0, sizeof(clientAddress));
  clientAddrSize = sizeof(clientAddress);
  int N=*(int *)n;
  while(N--){
    clientSocketFd = accept(serverSocketFd, (struct sockaddr *)&clientAddress, &clientAddrSize);
    send(clientSocketFd,buf,strlen(buf)+1,0);
    
    memset(buf,0,sizeof buf);
    recv(clientSocketFd,buf,1024,0);

    close(clientSocketFd);
  }
  dup2(stdfd,1);
  fprintf(stdout,"%s",buf);
  close(serverSocketFd);
  remove(SOCK_PATH);
  return 0;
}

void *client_thread(void* com){
  int socketFd = 0;
  struct sockaddr_un address;
  int result = 0;
  char buf[1024]={0};
  int addSize = 0;

  socketFd = socket(PF_LOCAL,SOCK_STREAM, 0);
   
  memset(&address, 0, sizeof(address));
  address.sun_family = AF_LOCAL;
  strcpy(address.sun_path, SOCK_PATH);
  addSize = sizeof(address);
  result = connect(socketFd,(struct sockaddr *)&address, addSize);

  if(result == -1){
    fprintf(stderr,"client : ERROR : Connection Failed\n");
    return (void *)-1;
  }
  int fd[2];  pipe(fd);
  int pid=fork(); 
  int status;
  if(pid==0){
    recv(socketFd,buf,1024,0);
    FILE *fp=fopen("temp","w");
    fprintf(fp,"%s",buf);
    fclose(fp);
    freopen("temp","r",stdin);
    dup2(fd[1],1);
    exec((struct single_command*)com);
    fprintf(stderr, "%s: command not found\n", ((struct single_command*)com)->argv[0]);
    exit(1);
  }
  waitpid(pid,&status,0);
  if(status!=0) return (void *)-1;
  memset(buf,0,sizeof buf);
  read(fd[0],buf,1024);
  write(socketFd,buf,strlen(buf)+1); 
  close(socketFd);
  remove("temp");
  return 0;
}
void free_commands(int n_commands, struct single_command (*commands)[512])
{
  for (int i = 0; i < n_commands; ++i) {
    struct single_command *com = (*commands) + i;
    int argc = com->argc;
    char** argv = com->argv;

    for (int j = 0; j < argc; ++j) {
      free(argv[j]);
    }

    free(argv);
  }

  memset((*commands), 0, sizeof(struct single_command) * n_commands);
}
