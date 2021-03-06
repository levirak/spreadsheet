#ifndef __sc_mem__
#define __sc_mem__

#include <stddef.h>

#include <fcntl.h>

#define DEFAULT_CELL_WIDTH 16
#define MAX_COLUMN_WIDTH 1024 /* Necessary? */

enum document_property_flags {
    DOC_PRINT_TOP      = 0x01,
    DOC_PRINT_SIDE     = 0x02,
    DOC_PRINT_WIDTH    = 0x04,
    DOC_PRINT_HEAD_SEP = 0x08,
};

enum column_align {
    ALIGN_LEFT = 0, /* default */
    ALIGN_CENTER,
    ALIGN_RIGHT,
    /* TODO: ALIGN_DECIMAL? */
};

typedef struct cell_value {
    enum cell_type {
        CELL_TYPE_NONE = 0,
        CELL_TYPE_STRING,
        CELL_TYPE_REAL,
        CELL_TYPE_INT,
        CELL_TYPE_EXPR,
    } Type;

    union {
        char *AsString;
        f64 AsReal;
        s64 AsInt;
        struct expr *AsExpr;
    };
} cell_value;

typedef struct cell {
    enum cell_status_flags {
        CELL_EVALUATING  = 0x01,
        CELL_CLOSE_CYCLE = 0x02,
    } Status;

    enum cell_error_code {
        ERROR_NONE = 0,
        ERROR_NOFILE,
        ERROR_NOREF,
        ERROR_UNCLOSED,
        ERROR_CYCLE,
        ERROR_RANGE,
        ERROR_SUB,
        ERROR_TRAIL,
        ERROR_BAD_FUNC,
    } ErrorCode;

    cell_value Value;
} cell;

typedef struct column {
    s32 Width;
    s32 Align;
    s32 CellCap;
    s32 CellCount;
    cell *Cell;
} column;

typedef struct document {
    s32 ColumnCap;
    s32 ColumnCount;
    column *Column;

    struct page *Strings;

    s32 Properties;
    fd DirFD;
    s32 HeadSepIdx;
} document;

void *PushSize(document *Document, umm Size, umm Align);
#define INITIAL_STRING_STACK_SIZE 1024 /* TODO: figure out a better number */
char *PushString(document *Document, char *String);

#define ReadDocument(P) ReadDocumentRelativeTo(NULL, P)
document *ReadDocumentRelativeTo(document *Document, char *FileName);
void FreeDocument(document *);

column *GetColumn(document *Doc, s32 ColumnIndex);
cell *GetCell(document *Doc, s32 ColumnIndex, s32 RowIndex);

#define MemZero(M, S) MemSet(M, S, 0)
void MemSet(void *Destination, umm Size, char Byte);
void MemCopy(void *Destination, umm Size, void *Source);

typedef struct file {
    fd Handle;
    u32 Error;
    u32 BufSz;
    u32 BufPos;
    char Buffer[256];
} file;

s32 Read(file *File, char *Buffer, umm Size);

#endif
