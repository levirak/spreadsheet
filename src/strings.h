#ifndef __sc_strings__
#define __sc_strings__

#include "mem.h"

#include <stdlib.h>
#include <stdio.h>

smm GetLine(char *Buf, mm BufSz, file *File);
s32 GlyphCount(char *Str);
mm StringLength(char *Str);
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

char *CellValueToString(cell_value Value, char *Buffer, mm Size);
bool LooksLikeInt(char *Str);
bool LooksLikeReal(char *Str);

#endif
