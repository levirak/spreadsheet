#ifndef __sc_mem__
#define __sc_mem__

#include <stddef.h>

typedef struct cell {
    char Value[32]; /* TEMP? */
    struct cell *Next;
} cell;

typedef struct row {
    struct cell *FirstCell;
    struct row *Next;
} row;

row *AllocRow();
void FreeRow(row *);

cell *AllocCell(cell *Prototype);
void FreeCell(cell *);

#define MemZero(M, S) MemSet(M, S, 0)
void MemSet(void *Destination, size_t Size, char Byte);
void MemCopy(void *Destination, size_t Size, void *Source);

#endif
