/*
 * Main source code file for lsh shell program
 *
 * You are free to add functions to this file.
 * If you want to add functions in a separate file
 * you will need to modify Makefile to compile
 * your additional functions.
 *
 * Add appropriate comments in your code to make it
 * easier for us while grading your assignment.
 *
 * Submit the entire lab1 folder as a tar archive (.tgz).
 * Command to create submission archive:
      $> tar cvf lab1.tgz lab1/
 *
 * All the best
 */


#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "parse.h"

#define READ_END 0
#define WRITE_END 1
#define BUFFER_S 256

/*
 * Function declarations
 */

void PrintCommand(int, Command *);
void PrintPgm(Pgm *);
void stripwhite(char *);
void RunCommandRec(Command *);
void RunSingleCommand(Pgm *, int, int, int);
void HandleInterrupt(int sig);

/* When non-zero, this global means the user is done using this program. */
int done = 0;
const char *PATH[] = {"/usr/bin:/bin", NULL};

/*
 * Name: main
 *
 * Description: Gets the ball rolling...
 *
 */
int main(void)
{
  Command cmd;
  int n;

  signal(SIGINT, HandleInterrupt);
  while (!done) {

    char *line;
    line = readline("> ");

    if (!line) {
      /* Encountered EOF at top level */
      done = 1;
    }
    else {
      /*
       * Remove leading and trailing whitespace from the line
       * Then, if there is anything left, add it to the history list
       * and execute it.
       */
      stripwhite(line);

      if(*line) {
        add_history(line);
        /* execute it */
        n = parse(line, &cmd);
        PrintCommand(n, &cmd);
        if(n){
          RunCommandRec(&cmd);
        }else{
          printf("Invalid command, %d returned by parser\n", n);
        }
      }
    }

    if(line) {
      free(line);
    }
  }
  return 0;
}

/*
 * Name: PrintCommand
 *
 * Description: Prints a Command structure as returned by parse on stdout.
 *
 */
void
PrintCommand (int n, Command *cmd)
{
  printf("Parse returned %d:\n", n);
  printf("   stdin : %s\n", cmd->rstdin  ? cmd->rstdin  : "<none>" );
  printf("   stdout: %s\n", cmd->rstdout ? cmd->rstdout : "<none>" );
  printf("   bg    : %s\n", cmd->background ? "yes" : "no");
  PrintPgm(cmd->pgm);
}

/*
 * Name: PrintPgm
 *
 * Description: Prints a list of Pgm:s
 *
 */
void
PrintPgm (Pgm *p)
{
  if (p == NULL) {
    return;
  }
  else {
    char **pl = p->pgmlist;

    /* The list is in reversed order so print
     * it reversed to get right
     */
    PrintPgm(p->next);
    printf("    [");
    while (*pl) {
      printf("%s ", *pl++);
    }
    printf("]\n");
  }
}

/*
 * Name: stripwhite
 *
 * Description: Strip whitespace from the start and end of STRING.
 */
void
stripwhite (char *string)
{
  register int i = 0;

  while (isspace( string[i] )) {
    i++;
  }

  if (i) {
    strcpy (string, string + i);
  }

  i = strlen( string ) - 1;
  while (i> 0 && isspace (string[i])) {
    i--;
  }

  string [++i] = '\0';
}

void
RunCommandRec(Command *cmd)
{
  Pgm *p = cmd->pgm; // Handle several pgms in case of piping
  int in=0,out=1;
  if(cmd->rstdin){
    in = open(cmd->rstdin, O_RDONLY);
    printf("using cmd->stdin \n");
  }
  if(cmd->rstdout){
    //use as output file(?)
    //open file and use as in
    out = open(cmd->rstdout, O_CREAT | O_APPEND | O_WRONLY);
    //O_CREAT, create file if it doesnt exist
    //O_APPEND, append to file
    //O_WRONLY, write only
  }

  RunSingleCommand(p, in, out, cmd->background);
}

void
RunSingleCommand(Pgm *p, int fdin, int fdout, int background)
{
  // args[0] will be command
  char **args=p->pgmlist;
  char *dir;
  int fd[2];
  int pipe_open=0;
  int *status;

  if(!strcmp(*args, "cd"))
  {
    *args++;
    if(*args == NULL){
      dir = getenv("HOME");
    }else{
      sprintf(dir, "%s", *args);
    }
    printf("Changing dir to: %s \n", dir);

    if(chdir(dir) == -1)
    {
      printf("failed to change dir to %s\n", dir);
    }
  }
  else if(!strcmp(*args, "exit"))
  {
    close(0);
  } else
  {
    if(p->next){
      // we have another command to run
      // create pipe
      if(pipe(fd) == -1){
        fprintf(stderr, "Pipe Failed\n");
        return;
      }else{
        pipe_open = 1;
      }
    }
    //run command
    pid_t pid = fork();
    // debug
    //int pid = 0;
    if(pid == 0)
    {

      if(background)
      {
        setpgid(0,0);
      }

      //child
      if(p->next != NULL)
      {
        close(fd[WRITE_END]);
        // redirect READ_END of pipe to stdin
        dup2(fd[READ_END], fdin);
        close(fd[READ_END]);
      }else{
        dup2(fdin, 0);
      }
      //send output (stdout) to fdout
      dup2(fdout, 1);

      if(execvp(args[0], args) == -1)
      {
        printf("\nfailure, errno: %d\n", errno);
      }
    }

    if(p->next){
      RunSingleCommand(p->next, fdin, fd[WRITE_END], background);
      close(fd[WRITE_END]);
    }

    if(background == 0){
      waitpid(pid, &status, 0);
    }

  }

}
void
HandleInterrupt(int sig)
{
  if(sig == SIGINT){
    printf("interrupt recieved \n");
  }
}
