#ifndef __sc_strings__
#define __sc_strings__

#include <stdlib.h>
#include <stdio.h>

ssize_t GetLine(char *buf, size_t bufsz, FILE *file);
int GlyphCount(char *str);
char *SkipWord(char *str);
char *SkipSpaces(char *str);
char *BreakOffWord(char *str);
char *BreakOffCell(char *str);
int CompareString(char *a, char *b);
char *Strip(char *str);
void PrintStringCell(char *cell, char *delim, int column);
void PrintNumCell(int cell, char *delim, int column);

#endif
