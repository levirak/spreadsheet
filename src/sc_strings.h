#ifndef __sc_strings__
#define __sc_strings__

#include <stdlib.h>
#include <stdio.h>

ssize_t GetLine(char *Buf, size_t BufSz, FILE *File);
int GlyphCount(char *String);
char *SkipWord(char *String);
char *SkipSpaces(char *String);
char *BreakOffWord(char *String);
#define BreakOffCell(S) BreakAtChar((S), DELIM_CHAR)
char *BreakAtChar(char *String, char Delim);
char *FindChar(char *String, char Delim);
int CompareString(char *A, char *B);
char *Strip(char *String);
void PrintStringCell(char *Cell, char *Delim, int Column);
void PrintNumCell(int Cell, char *Delim, int Column);

size_t BufferString(char *Buffer, size_t Size, char *String);

int StringToPositiveInt(char *String);

#endif
