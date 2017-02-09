/* Alvin Ung
 * November 29, 2016
 * CS 352
 * Assignment 6
 */

/* $Id: arg_parse.c,v 1.6 2016/12/02 15:46:39 unga2 Exp $ */

#include "proto.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

/* Swaps 2 pointer values */
void swap(int *a, int *b) {
    int t;

    t = *a;
    *a = *b;
    *b = t;
}

/* Function to parse line buffer from user.
   Takes arguments: line buffer from user, argument count pointer.
   Returns an array of pointers to each argument and sets argument count. */
char **arg_parse (char *line, int *argcp) {
    int numArgs = 0;            // Number of arguments found
    int index = 0;              // Index of character in line buffer
    int argIndex = 0;           // Index to first character of an argument
    int quoteCount = 0;         // Number of double quotes found
    int character;              // Index of character in an argument
    int counter;                // Keeps track of how many additional indexes to shift

    /* Loop to count number of arguments */
    while (line[index] != '\0' && line[index] != 0) {
        /* Skip leading spaces */
        while (line[index] == ' ') {
            index++;
        }

        /* Found an argument and skips indexes until next space */
        if (line[index] != ' ' && line[index] != 0 && line[index] != '\0') {
            if (quoteCount % 2 == 0) {
                numArgs++;
            }
            if (line[index] == '\"') {
                quoteCount++;
            }
            index++;
            /* Skips indexes until next space while looking for double quotes */
            while (line[index] != ' ' && line[index] != 0 && line[index] != '\0') {
                if (line[index] == '\"') {
                    quoteCount++;
                }
                index++;
            }
        }
    }

    /* Found odd number of quotes */
    if (quoteCount % 2 != 0) {
        fprintf(stderr, "Found odd number of quotes\n");
        return NULL;
    }

    /* Parsing arguments */
    char **argsAry;
    argsAry = (char**) malloc(sizeof(char*) * (numArgs + 1));
    index = 0;

    /* check of malloc pointer is null */
    if (argsAry == NULL) {
        printf("\nmalloc pointer is null\n");
        return 0;
    }

    /* Storing number of arguments found for future use */
    swap(&numArgs, argcp);

    /* Storing argument pointers into malloced area */
    while (line[index] != 0) {
        while (line[index] == ' ') {
            index++;
        }
        if (line[index] != ' ' && line[index] != 0) {
            if (quoteCount % 2 == 0) {
                argsAry[argIndex] = &line[index];
                argIndex++;
            }
            if (line[index] == '\"') {
                quoteCount++;
            }
        }
        while (line[index] != ' ' && line[index] != 0) {
            index++;
            if (line[index] == '\"') {
                quoteCount++;
            }
        }
        /* Setting 0 character after an argument */
        if (line[index] == ' ' && quoteCount % 2 == 0) {
            line[index] = 0;
            index++;
        }
    }
    argsAry[argIndex] = 0;

    /* Removing double quotes from argsAry */
    if (argcp != 0) {
        /* Loop through each argument in argsAry */
        for (int i = 0; i < *argcp; i++) {
            character = 0;
            /* Scan through argument i */
            while (argsAry[i][character] != 0) {
                if (argsAry[i][character] == '\"') {
                    counter = character;
                    /* Remove double quote and shift rest of characters up 1 index */
                    while (argsAry[i][counter] != 0) {
                        argsAry[i][counter] = argsAry[i][counter + 1];
                        counter++;
                    }
                }
                character++;
            }
        }
    }
    return argsAry;
}