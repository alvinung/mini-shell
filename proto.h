/* Alvin Ung
 * November 29, 2016
 * CS 352
 * Assignment 6
 */

/* $Id: proto.h,v 1.9 2016/12/02 15:46:39 unga2 Exp $ */

#include <sys/stat.h>

/* Prototypes */
int processline (char *line, int readFd, int writeFd, int flag);
char **arg_parse (char *line, int *argcp);
int findBuiltin (char *args);
void exBuiltin (char **arg, int numArgs);
int expand (char *orig, char *new, int newsize);
void strmode(mode_t mode, char *p);
