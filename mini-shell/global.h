/* Alvin Ung
 * November 29, 2016
 * CS 352
 * Assignment 6
 */

 /* $Id: global.h,v 1.8 2016/12/02 15:46:39 unga2 Exp $ */

/*  */
#ifndef MAIN

int parensCount;
int exitVal;
int numberArgs;
int shiftVal;
int mArgc;
char **mArgv;

#else

extern int parensCount;
extern int exitVal;
extern int numberArgs;
extern int shiftVal;
extern int mArgc;
extern char **mArgv;

#endif