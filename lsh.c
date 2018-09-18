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
void RunCommand(Command *);

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
          RunCommand(&cmd);
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
RunCommand(Command *cmd)
{
  Pgm *p = cmd->pgm; // Handle several pgms in case of piping
  char **pl = p->pgmlist;
  char dir[80] = "";
  int fd[2];
  int pipe_open=0;

  if(!strcmp(*pl, "cd"))
  {
    *pl++;
    sprintf(dir, "%s", *pl++);
    printf("Changing dir to: %s \n", dir);

    if(chdir(dir) == -1)
    {
      printf("failed to change dir to %s\n", dir);
    }
  }
  else
  {
    while(p){
      if(p->next){
        printf("Opening pipe for command: %s \n", *pl);
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
      if(pid == 0){
        if(p->next){
          printf("Redirecting pipe to stdin for %s\n", *pl);
          close(fd[WRITE_END]);
          // redirect READ_END of pipe to stdin
          dup2(0, fd[READ_END]);
          close(fd[READ_END]);
        }else if(pipe_open==1){
          printf("Redirecting stdout to pipe for %s\n", *pl);
          printf("ASDASD");

          close(fd[READ_END]);
          // redirect stdout to WRITE_END of pipe
          dup2(fd[WRITE_END], 1);
          close(fd[WRITE_END]);
          pipe_open = 0;
        }
        //child
        printf("Running command %s .. \n", *pl);
        if(execvp(pl[0], pl) != -1){
          printf("sucess\n");
        }else{
          printf("\nfailure, errno: %d\n", errno);
        }
      }else{
        //parent
        if(cmd->background == 1){
          printf("running %s in background \n", *pl);
        }else if(pipe_open == 0){
          wait(NULL);
        }
      }
      p = p->next;
      pl = p->pgmlist;
    }
  }
}
