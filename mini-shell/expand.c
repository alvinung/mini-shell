/* Alvin Ung
 * November 29, 2016
 * CS 352
 * Assignment 6
 */

/*	$Id: expand.c,v 1.38 2016/12/02 15:46:39 unga2 Exp $	*/

#include "proto.h"
#include "global.h"
#include <dirent.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>


/* Function processes original buffer line and performs "expansions"
   on the original line. Checks for buffer overflows.
   Returns new expanded line
   Takes arguments: original line, new line, size of line */
int expand (char *orig, char *new, int newsize) {
	int origIndex = 0;						// Index of line being read
	int newIndex = 0;						// Index of line being written to
	int tempIndex = 0;						// Index used for character shifting
	int d_nameIndex;						// Index of filename characters
	char *temp;								// Array to store environment variable name
	char *eVar = 0;							// Array used to store environment variable value
	char pid[8];							// pid string
	char argsCount[4];						// number of arguments as string
	char exitNum[6];						// exit value of last command
	char *ctxArr;							// pointer context for wildcard expansion
	char *subCmd;							// pointer to subcommand for expansion

	while (orig[origIndex] != 0) {
		/* Check for buffer overflow */
		if (newIndex > newsize - 1) {
			return -2;
		}
		/* Check if character is beginning of an environment variable */
		if (orig[origIndex] == '$' && orig[origIndex + 1] == '{') {
			tempIndex = 0;
			origIndex += 2;
			temp = &orig[origIndex];
			while (orig[origIndex] != '}') {
				/* Reached end of line before finding closing brace */
				if (orig[origIndex] == 0) {
					return -1;
				}
				origIndex++;
				tempIndex++;
			}
			origIndex++;
			temp[tempIndex] = 0;
			eVar = getenv(temp);
		}
		/* Checks if environment variable exists or not */
		if (eVar != 0) {
			for (int i = 0; i < strlen(eVar); i++) {
				new[newIndex] = eVar[i];
				newIndex++;
			}
			eVar = 0;
		}
		/* Special variable processing with base 10 ASCII equivalent of shell's pid */
		if (orig[origIndex] == '$' && orig[origIndex + 1] == '$') {
			snprintf(pid, strlen(pid), "%d", getppid());
			for (int i = 0; i < strlen(pid); i++) {
				new[newIndex] = pid[i];
				newIndex++;
			}
		}
		/* Processing $n pattern, n = integer >= 0 */
		if (orig[origIndex] == '$' && isdigit(orig[origIndex + 1])) {
			int num;
			char number[15];
			memset(number, ' ', 15);
			origIndex++;

			/* If there's only 1 argument, $0 = name of shell */
			if (mArgc == 1 && orig[origIndex] == '0') {
				for (int i = 0; i < strlen(mArgv[0]); i++) {
					new[newIndex] = mArgv[0][i];
					newIndex++;
				}
				origIndex++;
			}
			/* There are multiple arguments */
			else {
				while (isdigit(orig[origIndex])) {
				number[tempIndex] = orig[origIndex];
				tempIndex++;
				origIndex++;
				}
				num = atoi(number);
				if (num + shiftVal + 1 < mArgc) {			// n is equal or less than # of args
					for (int i = 0; i < strlen(mArgv[num + shiftVal + 1]); i++) {
						if (num + shiftVal + 1 < mArgc) {
							/* Making sure $0 is always script name */
							if (num + 1 == 1) {
								new[newIndex] = mArgv[num + 1][i];
								newIndex++;
							}
							else {
								new[newIndex] = mArgv[num + shiftVal + 1][i];
								newIndex++;
							}
						}
					}
				}
				else {						// n is greater than # of args
					new[newIndex] = orig[origIndex];
					origIndex++;
					newIndex++;
				}
			}
		}
		/* Processing $# pattern, replaces with base 10 ASCII rep of # of args */
		if (orig[origIndex] == '$' && orig[origIndex + 1] == '#') {
			if (mArgc == 1) {
				new[newIndex] = '1';
				newIndex++;
				origIndex += 2;
			}
			else {
				snprintf(argsCount, sizeof(argsCount), "%d", numberArgs);
				for (int i = 0; i < strlen(argsCount); i++) {
					new[newIndex] = argsCount[i];
					newIndex++;
				}
				origIndex += 2;
			}
		}
		/* Processing $? pattern, replaces with base 
		10 ASCII rep of exit value of last command */
		if (orig[origIndex] == '$' && orig[origIndex + 1] == '?') {
			snprintf(exitNum, sizeof(exitNum), "%d", exitVal);
			for (int i = 0; i < strlen(exitNum); i++) {
				new[newIndex] = exitNum[i];
				newIndex++;
			}
			origIndex += 2;
		}
		/* Processing escape character for \* */
		if (orig[origIndex] == '\\' && orig[origIndex + 1] == '*') {
			new[newIndex] = '*';
			newIndex++;
			origIndex += 2;
		}
		/* Changing " to space if found before wildcard character */
		if (orig[origIndex] == '\"' && orig[origIndex + 1] == '*') {
			new[newIndex] = ' ';
			newIndex++;
			origIndex++;
		}
		/* Processing for wildcard expansion */
		if (orig[origIndex] == '*') {
			DIR *dir;
			struct dirent *dirP = malloc(sizeof(struct dirent));
			dir = opendir(".");
			int len;
			int ctxLen = 0;
			int checkNotFound;
			int tempOrigIndex;
			char tempChar;

			if (dir != NULL) {
				/* Complex expansion */
				if (orig[origIndex] == '*' && orig[origIndex + 1] != '\0') {
					tempIndex = 0;
					tempOrigIndex = origIndex;
					checkNotFound = newIndex;
					origIndex++;
					/* retrieving context */
					while (orig[origIndex] != ' ' && orig[origIndex]!= '\"' && orig[origIndex] != '\0') {
						/* Checking for / character in context */
						if (orig[origIndex] == '/') {
							closedir(dir);
							return -3;
						}
						ctxArr = &orig[origIndex];
						origIndex++;
						ctxLen++;
					}
					tempChar = orig[origIndex];
					orig[origIndex] = 0;
					/* Listing files that match context */
					while ((dirP = readdir(dir)) != NULL) {
						d_nameIndex = 0;
						len = strlen(dirP->d_name);
						if (dirP->d_name[0] != '.' && strcmp(&dirP->d_name[len - ctxLen], ctxArr) == 0) {							
							while (dirP->d_name[d_nameIndex] != '\0') {
								new[newIndex] = dirP->d_name[d_nameIndex];
								d_nameIndex++;
								newIndex++;
							}
							new[newIndex] = ' ';
							newIndex++;
						}
					}
					orig[origIndex] = tempChar;
					/* Files not found. Expand original argument */
					if (checkNotFound == newIndex) {
						while (orig[tempOrigIndex] != 0) {
							new[newIndex] = orig[tempOrigIndex];
							newIndex++;
							tempOrigIndex++;
						}
					}
					closedir(dir);
				}
				/* Simple expansion */
				else {
					while ((dirP = readdir(dir)) != NULL) {
						d_nameIndex = 0;
						if (dirP->d_name[0] != '.') {
							while (dirP->d_name[d_nameIndex] != '\0') {
								new[newIndex] = dirP->d_name[d_nameIndex];
								d_nameIndex++;
								newIndex++;
							}
							new[newIndex] = ' ';
							newIndex++;
						}
					}
					origIndex++;
					closedir(dir);
				}
			}
			else {
				/* error file could not open */
				printf("Error: File could not be opened\n");
			}
			free(dirP);
		}
		/* Command output expansion */
		if (orig[origIndex] == '$' && orig[origIndex + 1] == '(') {
			int parensIndex;
			int status;
			int cpid;
			int pfd[2];
			int rBytes;
			char buf[newsize];
			memset(buf, 0, newsize);
			tempIndex = 0;
			parensCount++;
			origIndex += 2;
			subCmd = &orig[origIndex];
			while (orig[origIndex] != 0) {
				if (orig[origIndex] == '(') {
					parensCount++;
				}
				else if (orig[origIndex] == ')') {
					parensCount--;
					if (parensCount == 0) {
						orig[origIndex] = 0;
						parensIndex = origIndex;
					}
				}
				else if (orig[origIndex] == 0 && parensCount != 0) {
					printf("Error: No matching )\n");
				}
				if (orig[origIndex] != 0) {
					origIndex++;
				}
			}
			/* Piping subcommand to child process */
			pipe(pfd);
			cpid = processline(subCmd, pfd[0], pfd[1], 0);
			orig[parensIndex] = ')';
			if (cpid == -1) {
				printf("Error: processline\n");
				return -4;
			}
			/* Processline success! */
			if (cpid >= 0) {
				close(pfd[1]);
				rBytes = read(pfd[0], buf, newsize);

				/* Read until end of file */
				while (rBytes > 0) {
					rBytes = read(pfd[0], buf, newsize);
				}
				if (rBytes < 0) {
					perror("read");
				}

				/* Wait on child if still running */
				if (cpid > 0) {
					waitpid(cpid, &status, 0);
					exitVal = WEXITSTATUS(status);
				}
		
				/* add subcommand to orig */
				while (buf[tempIndex] != 0) {
					new[newIndex] = buf[tempIndex];
					newIndex++;
					tempIndex++;
				}
				close(pfd[0]);
				
				tempIndex = 0;
				while (tempIndex < (newIndex)) {
					if (new[tempIndex] == '\n') {
						new[tempIndex] = ' ';
					}
					tempIndex++;
				}
				newIndex = tempIndex;
			}
		}
		if (orig[origIndex] == ')' && parensCount == 0) {
			origIndex++;
		}
		new[newIndex] = orig[origIndex];
		origIndex++;
		newIndex++;
	}
	new[newIndex] = 0;
	return 0;
}