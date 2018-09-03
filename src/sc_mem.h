#ifndef __sc_mem__
#define __sc_mem__

#include <stddef.h>

#define DEFAULT_CELL_WIDTH 8

typedef struct cell {
    int Width;
    char Value[32]; /* TEMP? */
} cell;

typedef struct row {
    cell *Cell;
    int CellCap;
    int CellCount;
} row;

typedef struct document {
    row *Row;
    int RowCap;
    int RowCount;
} document;

document *AllocDocument();
void FreeDocument(document *);

row *GetNewRow(document *Root);
cell *GetNewCell(row *Row);

#define MemZero(M, S) MemSet(M, S, 0)
void MemSet(void *Destination, size_t Size, char Byte);
void MemCopy(void *Destination, size_t Size, void *Source);

#endif
