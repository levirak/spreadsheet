#ifndef __sc_strings__
#define __sc_strings__

#include <stdlib.h>
#include <stdio.h>

ssize_t GetLine(char *Buf, size_t BufSz, int FileHandle);
int GlyphCount(char *Str);
size_t StringSize(char *Str);
char *SkipWord(char *Str);
char *SkipSpaces(char *Str);
char *BreakOffWord(char *Str);
#define BreakOffCell(S) BreakAtChar((S), DELIM_CHAR)
char *BreakAtChar(char *Str, char Delim);
char *BreakAtLastChar(char *Str, char Delim);
char *FindChar(char *Str, char Delim);
int CompareString(char *A, char *B);
char *Strip(char *Str);
void PrintStringCell(char *Cell, char *Delim, int Column);
void PrintNumCell(int Cell, char *Delim, int Column);

size_t BufferString(char *Buffer, size_t Size, char *Str);

int StringToPositiveInt(char *Str);
int StringToInt(char *Str, char **RHS);
float StringToReal(char *Str, char **RHS);

#endif
