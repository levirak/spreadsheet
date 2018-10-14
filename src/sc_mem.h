#ifndef __sc_mem__
#define __sc_mem__

#include <stddef.h>

#include <fcntl.h>

#define DEFAULT_CELL_WIDTH 8

enum cell_status_flags {
    CELL_FUNCTION    = 0x01,
    CELL_EVALUATING  = 0x02,
    CELL_ERROR       = 0x04,
    CELL_CAUSE_ERROR = 0x08,
};

enum document_property_flags {
    DOC_PRINT_TOP  = 0x01,
    DOC_PRINT_SIDE = 0x02,
};

typedef struct cell {
    int Status;
    int Width;
    char Value[32]; /* TEMP? */
} cell;

typedef struct row {
    cell *Cell;
    int CellCap;
    int CellCount;
} row;

typedef struct document {
    int RowCap;
    int RowCount;
    row *Row;

    int Properties;
    int DirFD;

    /* @TEMP: this row count
    * Note that for now there can be now more than 26 rows.
    */
    int ColWidth[8];
} document;


#define ReadSheet(P) ReadSheetAt(AT_FDCWD, P)
document *ReadSheetAt(int FD, char *FileName);
void FreeDocument(document *);

row *GetNewRow(document *Root);
cell *GetNewCell(row *Row);

#define MemZero(M, S) MemSet(M, S, 0)
void MemSet(void *Destination, size_t Size, char Byte);
void MemCopy(void *Destination, size_t Size, void *Source);

#endif
