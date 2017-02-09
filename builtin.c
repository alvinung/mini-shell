/* Alvin Ung
 * November 29, 2016
 * CS 352
 * Assignment 6
 */

/* $Id: builtin.c,v 1.19 2016/12/02 15:46:39 unga2 Exp $ */

#include "proto.h"
#include "global.h"
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>


int builtinCount = 9;						// Number of built-in functions
int func = 0;								// Integer to determine what built-in function was called
char *biList[9] = {"exit", "aecho",			// Array of built-in function names
				   "envset", "envunset",
				   "cd", "shift",
				   "unshift", "sstat",
				   "read"};

/* Built-in function "exit"
   Calls the exit(3) library call
   Takes arguments: array of arguments, number of arguments */
void bi_exit (char **arg, int numArgs) {
	if (numArgs == 1) {
		exitVal = 0;
		exit(0);
	}
	else {
		int val = atoi(arg[1]);
		exitVal = val;
		exit(val);
	}
}

/* Built-in function "aecho"
   Echo's user input
   Takes arguments: array of arguments, number of arguments */
void bi_aecho (char **arg, int numArgs) {
	/* Calling aecho with no arguments */
	if (numArgs == 1) {
		dprintf(1, "%c", '\n');
	}
	/* Called with -n flag. No new line character to print */
	else if (strcmp(arg[1], "-n") == 0) {
		for (int i = 2; i < numArgs; i++) {
			dprintf(1, "%s", arg[i]);
			if (i != numArgs - 1) {
				dprintf(1, "%c", ' ');
			}
		}
	}
	/* Echo user input normally */
	else {
		for (int i = 1; i < numArgs; i++) {
			dprintf(1, "%s", arg[i]);
			if (i != numArgs - 1) {
				dprintf(1, "%c", ' ');
			}
		}
		dprintf(1, "%c", '\n');
	}
	exitVal = 0;
}

/* Sets the environment variable of the same name to given value
   Takes argument: array of arguments */
void bi_envset (char **arg) {
	setenv(arg[1], arg[2], 1);
	exitVal = 0;
}

/* Removes enviroment variable from current environment
   Takes argument: array of arguments */
void bi_envunset (char **arg) {
	unsetenv(arg[1]);
	exitVal = 0;
}

/* Change working directory
   Takes argument: array of arguments */
void bi_cd (char **arg) {
	int status;
	/* Missing PATH argument, try HOME environment variable */
	if (arg[1] == NULL) {
		status = chdir(getenv("HOME"));
		exitVal = 0;
	}
	else {
		status = chdir(arg[1]);
		exitVal = 0;
	}
	/* Error: not a directory */
	if (status != 0) {
		exitVal = 1;
		perror("cd");
	}
}

/* Shift arguments to main by specified amount
   Takes argument: array of arguments */
void bi_shift(char **arg) {
	/* No shift argument: shift 1 */
	if (arg[1] == NULL) {
		/* Checking if shifting is possible */
		if (1 > numberArgs - 1) {
			printf("Error: Cannot perform shift\n");
			exitVal = 1;
		}
		else {
			shiftVal++;
			numberArgs--;
			exitVal = 0;
		}
	}
	/* Shifting by given amount */
	else {
		/* Checking if shifting is possible */
		if (atoi(arg[1]) > numberArgs - 1) {
			printf("Error: Cannot perform shift\n");
			exitVal = 1;
		}
		else {
			shiftVal += atoi(arg[1]);
			numberArgs -= atoi(arg[1]);
			exitVal = 0;
		}
	}
}

/* Unshifts arguments to main by specified amount
   Takes argument: array of arguments */
void bi_unshift(char **arg) {
	/* No unshift argument: reset shiftVal and numArgs */
	if (arg[1] == NULL) {
		shiftVal = 0;
		numberArgs = mArgc - 1;
		exitVal = 0;
	}
	/* Unshifting by given amount */
	else {
		/* Checking if unshifting is possible */
		if (shiftVal - atoi(arg[1]) < 0) {
			printf("Error: Cannot perform shift\n");
			exitVal = 1;
		}
		else {
			shiftVal -= atoi(arg[1]);
			numberArgs += atoi(arg[1]);
			exitVal = 0;
		}
	}
}

/* Displays file information for all files asked for
   Takes argument: array of arguments, number of arguments */
void bi_sstat(char **arg, int numArgs) {
	struct stat fileInfo;
	struct passwd *uName;
	struct group *gName;
	char perms[12];

	for (int i = 1; i < numArgs; i++) {
		stat(arg[i], &fileInfo);
		uName = getpwuid(fileInfo.st_uid);
		gName = getgrgid(fileInfo.st_gid);
		strmode(fileInfo.st_mode, perms);

		dprintf(1, "%s ", arg[i]);
		dprintf(1, "%s ", uName->pw_name);
		dprintf(1, "%s ", gName->gr_name);
		dprintf(1, "%s ", perms);
		dprintf(1, "%ld ", (long) fileInfo.st_nlink);
		dprintf(1, "%lld ", (long long) fileInfo.st_size);
		dprintf(1, "%s", ctime(&fileInfo.st_mtime));
	}
}

/* Sets the environment variable named in arg[1] to be a defined
   as a line of input from from stdin.
   Takes arguments: array of arguments, number of arguments */
void bi_read(char **arg, int numArgs) {
	if (numArgs != 2) {
		printf("Usage: read variable-name\n");
		exitVal = -1;
	}
	else {
		char buffer[1024];

		if (fgets(buffer, 1024, stdin) == buffer) {
			int len = strlen(buffer);
			if (buffer[len - 1] == '\n') {
				buffer[len - 1] = 0;
			}
			setenv(arg[1], buffer, 1);
			exitVal = 0;
		}
		else {
			exitVal = -1;
		}
	}
}

/* Determines if argument is a built-in function 
   Takes argument: arg[0] */
int findBuiltin (char *arg) {
	int result;
	int i;

	for (i = 0; i < builtinCount; i++) {
		result = strcmp(&arg[0], biList[i]);
		if (result == 0) {
			func = i + 1;
			return result;
		}
	}
	return -1;
}

/* Executes built-in function 
   Takes arguments: array of arguments, number of arguments */
void exBuiltin (char **arg, int numArgs) {
	// use switch statement with arg string
	switch(func) {
		case 1 :					// user input argument exit
			bi_exit(arg, numArgs);
			break;
		case 2 :					// user input argument aecho
			bi_aecho(arg, numArgs);
			break;
		case 3 :					// user input argument envset
			bi_envset(arg);
			break;
		case 4 :					// user input argument envunset
			bi_envunset(arg);
			break;
		case 5 :					// user input argument cd
			bi_cd(arg);
			break;
		case 6 :					// user input argument shift
			bi_shift(arg);
			break;
		case 7 :					// user input argument unshift
			bi_unshift(arg);
			break;
		case 8 :					// user input argument sstat
			bi_sstat(arg, numArgs);
			break;
		case 9 :					// user input argument read
			bi_read(arg, numArgs);
			break;
	}
}
