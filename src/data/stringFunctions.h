#ifndef STRING_FUNCS
#define STRING_FUNCS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

int numberOfDigits (double n);
char* jsonParser(char* jsonString, char* requestedField);
char *str_replace(char *orig, char *rep, char *with);

#endif
