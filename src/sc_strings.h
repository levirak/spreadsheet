#ifndef __sc_strings__
#define __sc_strings__

#include <sc_mem.h>

#include <stdlib.h>
#include <stdio.h>

ssize_t GetLine(char *Buf, mm BufSz, s32 FileHandle);
s32 GlyphCount(char *Str);
mm StringSize(char *Str);
char *SkipWord(char *Str);
char *SkipSpaces(char *Str);
char *BreakOffWord(char *Str);
#define BreakOffCell(S) BreakAtChar((S), DELIM_CHAR)
char *BreakAtChar(char *Str, char Delim);
char *BreakAtLastChar(char *Str, char Delim);
char *FindChar(char *Str, char Delim);
s32 CompareString(char *A, char *B);
char *Strip(char *Str);

void PrintCell(cell *Cell, char *Delim, s32 Width, s32 Align);
void PrintNumber(s32 Num, char *Delim, s32 Width, s32 Align);

mm BufferString(char *Buffer, mm Size, char *Str);
mm BufferSpaces(char *Buffer, mm Size, s32 Count);

s32 StringToPositiveInt(char *Str);
s32 StringToInt(char *Str, char **RHS);
r32 StringToReal(char *Str, char **RHS);

#endif
