#ifndef __sc_strings__
#define __sc_strings__

#include "mem.h"

#include <stdlib.h>
#include <stdio.h>

smm GetLine(char *Buf, umm BufSz, file *File);
s32 GlyphCount(char *Str);
umm StringLength(char *Str);
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

umm BufferString(char *Buffer, umm Size, char *Str);
umm BufferSpaces(char *Buffer, umm Size, s32 Count);

s32 StringToPositiveInt(char *Str);
s32 StringToInt(char *Str, char **RHS);
f64 StringToReal(char *Str, char **RHS);

char *CellValueToString(cell_value Value, char *Buffer, umm Size);
bool LooksLikeInt(char *Str);
bool LooksLikeReal(char *Str);

#endif
