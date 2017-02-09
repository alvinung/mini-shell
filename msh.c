/* Alvin Ung
 * November 29, 2016
 * CS 352
 * Assignment 6
 */

/* $Id: msh.c,v 1.30 2016/12/02 15:55:39 unga2 Exp $ */

#define MAIN

#include "proto.h"
#include "global.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>


/* Constants */ 

#define LINELEN 200000

/* Prototypes */

int processline (char *line, int readFd, int writeFd, int flag);
void signalHandler (int signal);

/* Shell main */

int main (int argc, char **argv) {
    char   buffer [LINELEN];
    int    len;
    int buffIndex;
    int quoteCount;
    FILE *process;

    /* Initializing global variables */
    mArgc = argc;
    mArgv = argv;
    numberArgs = argc - 1;
    shiftVal = 0;
    parensCount = 0;

    if (mArgc == 1) {
        process = stdin;
    }
    else {
        process = fopen(mArgv[1], "r");
        if (process == NULL) {
            perror("read");
            exit(127);
        }
    }

    while (1) {

        /* prompt and get line */
        if (mArgc == 1) {
            fprintf (stderr, "%% ");
        }
        if (fgets(buffer, LINELEN, process) != buffer) {
            break;
        }

        /* Get rid of \n at end of buffer. */
        len = strlen(buffer);
        if (buffer[len-1] == '\n') {
            buffer[len-1] = 0;
        }

        /* Check for comments # character */
        buffIndex = 0;
        quoteCount = 0;
        while (buffer[buffIndex] != 0) {
            if (buffer[buffIndex] == '\"') {
                quoteCount++;
            }
            if (buffer[buffIndex] == '#' && buffer[buffIndex - 1] != '$' && 
                quoteCount % 2 == 0) {
                buffer[buffIndex] = 0;
            }
            buffIndex++;
        }

        /* Run it ... */
        processline (buffer, 0, 1, 1);
    }

    if (!feof(stdin)) {
        perror ("read");
    }

    return 0;       /* Also known as exit (0); */
}

/* Function for handling signals */
void signalHandler (int signal) {
    /* Handle SIGINT */
}

/* Determines if input line contains a pipe character | 
   Takes argument: line expanded line entered by user */
int findPipe (char *line) {
    int lineIndex = 0;
    while (line[lineIndex] != 0) {
        if (line[lineIndex] == '\"') {
            lineIndex++;
            while (line[lineIndex] != '\"') {
                lineIndex++;
            }
        }
        if (line[lineIndex] == '|') {
            return 0;
        }
        lineIndex++;
    }
    return -1;
}

/* Function to kill zombie processes */
void killZombies () {
    while (wait(NULL) > 0);
}

/* Function to recognize and handle pipeline character | */
void pipeline (char *line, int readFd, int writeFd) {
    int fd[2];
    int fd2[2];
    char *cmd1;
    char *cmd2;
    int lineIndex = 0;
    int quoteCount = 0;

    cmd1 = line;

    while (line[lineIndex] != 0) {
        /* Find and split up commands for piping */
        if (line[lineIndex] == '\"') {
            quoteCount++;
        }
        if (line[lineIndex] == '|' && quoteCount % 2 == 0) {
            line[lineIndex] = 0;
            lineIndex++;

            /* Walk spaces */
            if (line[lineIndex] == ' ') {
                while (line[lineIndex] == ' ') {
                    lineIndex++;
                }
            }
            /* Set pointer to next command */
            cmd2 = &line[lineIndex];
            break;
        }
        lineIndex++;
    }

    /* Call pipe and process commands */
    pipe(fd);
    processline(cmd1, readFd, fd[1], 1);
    close(fd[1]);

    pipe(fd2);
    processline(cmd2, fd[0], fd2[1], 1);
    close(fd[0]);
    close(fd2[1]);
    killZombies();
}

/* Function to redirect stdin, stdout, and stderr */
void redirection (char *line, int *readFd, int *writeFd, int *errorFd) {
    int lineIndex = 0;
    char fd[3];
    int redirectingFd;
    int flag;
    char *file;
    char *redirectThis;
    int redirIndex = 0;

    fd[0] = -1;
    fd[1] = -2;
    fd[3] = -3;

    while (line[lineIndex] != 0) {
        /* Looking for unquoted redirection characters */
        if (line[lineIndex] == '\"') {
            lineIndex++;
            while (line[lineIndex] != '\"') {
                lineIndex++;
            }
        }
        if (line[lineIndex] == '<' || line[lineIndex] == '>' || line[lineIndex] == '2') {
            redirectThis = &line[lineIndex];
            if (line[lineIndex] == '<') {
                redirectingFd = 0;
            }
            else if (line[lineIndex] == '>') {
                redirectingFd = 1;
            }
            else if (line[lineIndex] == '2' && line[lineIndex + 1] == '>') {
                redirectingFd = 2;
                lineIndex++;
            }
            lineIndex++;

            /* appending or overwriting */
            if (line[lineIndex] == '>') {
                flag = O_APPEND;
                lineIndex++;
            }
            else {
                flag = O_TRUNC;
            }

            /* walk spaces */
            while (line[lineIndex] == ' ') {
                lineIndex++;
            }

            /* Find filename and terminate end of filename */
            file = &line[lineIndex];
            while (line[lineIndex] != ' ' && line[lineIndex] != 0) {
                lineIndex++;
            }
            line[lineIndex] = 0;

            /* close previously opened file if needed */
            if (fd[redirectingFd] >= 0) {
                close(fd[redirectingFd]);
            }

            /* open/create file for read/write */
            if (redirectingFd == 0) {
                fd[redirectingFd] = open(file, O_RDONLY);
            }
            else if (redirectingFd == 1) {
                fd[redirectingFd] = open(file, O_CREAT | O_WRONLY | flag, S_IRWXU | S_IRWXO);
            }
            else if (redirectingFd == 2) {
                fd[redirectingFd] = open(file, O_CREAT | O_WRONLY | flag, S_IRWXU | S_IRWXO);
            }

            /* Replace redirection with spaces */
            redirIndex = 0;
            while (redirectThis[redirIndex] != 0) {
                redirectThis[redirIndex] = ' ';
                redirIndex++;
            }
            redirectThis[redirIndex] = ' ';
        }
        lineIndex++;
    }
    /* Setting file descriptors */
    if (fd[0] != -1) {
        *readFd = fd[0];
    }
    else if (fd[1] != -2) {
        *writeFd = fd[1];
    }
    else if (fd[2] != -3) {
        *errorFd = fd[2];
    }
}

/* Function that does the processing of the shell.
   Takes argument: line buffer entered by user */
int processline (char *line, int readFd, int writeFd, int flag) {
    pid_t  cpid;                // Child ID after fork()
    int    status;              // Status of child process
    int argsCount;              // Number of arguments returned by arg_parse
    int isBuiltin;              // Value to determine if function is built-in
    int isPipe;                 // Value to determine if expanded line contains pipe character
    int expResult;              // Value to determine expansion failure/success
    char **args;                // Array of arguments
    char newLine [LINELEN];     // Array for expanded line    
    int cReadFd;                // Read file descriptor for child
    int cWriteFd;               // Write file descriptor for child
    int cErrorFd;               // Error file descriptor for child

    /* Initializing child file descriptors */
    cReadFd = readFd;
    cWriteFd = writeFd;
    cErrorFd = 2;

    /* Signal Handler */
    struct sigaction sigAct;
    sigAct.sa_handler = &signalHandler;
    sigAct.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sigAct, NULL);

    /* Expand line and check for environment variables */
    expResult = expand(line, newLine, LINELEN);
    /* Error: missing closing } */
    if (expResult == -1) {
        printf("msh: no matching }\n");
        return -1;
    }
    /* Error: buffer overflow */
    else if (expResult == -2) {
        printf("msh: Buffer overflow\n");
        return -1;
    }
    /* Error: found / charcter in wildcard context */
    else if (expResult == -3) {
        printf("msh: no match\n");
        return -1;
    }
    /* Error: child process returns with id of -1 */
    else if (expResult == -4) {
        printf("msh: processline error on subcommand\n");
        return -1;
    }
    /* Expansion success */
    else {
        isPipe = findPipe(newLine);
        if (isPipe >= 0) {
            pipeline(newLine, readFd, writeFd);
            killZombies();
            return 0;
        }
        redirection(newLine, &cReadFd, &cWriteFd, &cErrorFd);
        args = arg_parse(newLine, &argsCount);
    }

    /* If arg_parse finds no arguments, no nothing */
    if (argsCount == 0) {
        free(args);
        return -1;
    }

    /* Check if argument is a built-in function */
    isBuiltin = findBuiltin(args[0]);

    /* Start a new process to do the job if command is not a built-in function */
    if (isBuiltin == 0) {
        exBuiltin(args, argsCount);
        free(args);
        return 0;
    }
    else {
        cpid = fork();        
        if (cpid < 0) {
            perror ("fork");
            free(args);
            return cpid;
        }
    }
    
    /* Check for who we are! */
    if (cpid == 0) {
        /* We are the child! */

        /* Set read fd to read end of pipe */
        if (cReadFd != 0) {
            dup2(cReadFd, 0);
        }

        /* Set write fd to write end of pipe */
        if (cWriteFd != 1) {
            dup2(cWriteFd, 1);
        }
        execvp (args[0], args);
        perror ("exec");
        fclose(stdin);
        exit (127);
    }
    
    /* Have the parent wait for child to complete */
    if (flag) {
        if (wait (&status) < 0) {
            free(args);
            perror ("wait");
        }
        if (WIFEXITED(status)) {
            killZombies();
            exitVal = WEXITSTATUS(status);
        }
    }
    free(args);
    return cpid;
}