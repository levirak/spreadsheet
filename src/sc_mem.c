#include <main.h>
#include <sc_mem.h>
#include <dbg.h> /* TODO: is Zed Shaw's dbg.h good? */

#include <sc_strings.h>

#include <stdlib.h>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

static inline
document *AllocDocument() {
    document *Doc = malloc(sizeof *Doc);

#   define INITIAL_ROW_CAP 1

    *Doc = (document){
        .RowCap = INITIAL_ROW_CAP,
        .RowCount = 0,
        .Row = malloc(sizeof *Doc->Row * INITIAL_ROW_CAP),
    };

    for (size_t i = 0; i < ArrayCount(Doc->ColWidth); ++i) {
        Doc->ColWidth[i] = 8;
    }

    return Doc;
}

document *ReadDocumentRelativeTo(document *Doc, char *FileName) {
    char Buffer[1024];
    document *NewDocument;

    int DirFD = Doc? Doc->DirFD: AT_FDCWD;

    int FileHandle = openat(DirFD, FileName, O_RDONLY);
    if (FileHandle < 0) {
        return NULL;
    }

    NewDocument = AllocDocument();

    /* TODO: is Buffer big enough? */
    BufferString(Buffer, ArrayCount(Buffer), FileName);
    if (BreakAtLastChar(Buffer, '/') && CompareString(Buffer, ".") != 0) {
        if (*Buffer) {
            NewDocument->DirFD = open(Buffer, O_DIRECTORY | O_RDONLY);
        }
        else {
            NewDocument->DirFD = open("/", O_DIRECTORY | O_RDONLY);
        }
    }
    else if (DirFD == AT_FDCWD) {
        NewDocument->DirFD = AT_FDCWD;
    }
    else {
        NewDocument->DirFD = dup(DirFD);
    }

    while (GetLine(Buffer, ArrayCount(Buffer), FileHandle) > 0) {
        int Column = 0;
        char *RHS = Buffer;

        if (IsCommentChar(RHS[0])) {
            /* NOTE: at least for now, the comment character must be the first
            * character of the line to count as a comment */

            if (IsCommandChar(RHS[1])) {
                char *Word = RHS + 2;
                RHS = BreakOffWord(Word);

                /* process commands */

                if (CompareString(Word, "width") == 0) {
                    while (*RHS) {
                        char *Trailing;

                        RHS = BreakOffWord(Word = RHS);
                        int Width = StringToInt(Word, &Trailing);

                        if (Width > MAX_COLUMN_WIDTH) {
                            Error("Truncating overwide cell: %d (max %d)",
                                  Width, MAX_COLUMN_WIDTH);
                            Width = MAX_COLUMN_WIDTH;
                        }

                        if (*Trailing) {
                            Error("Trailing after width: %s", Word);
                        }

                        NewDocument->ColWidth[Column++] = Width;
                    }
                }
                else if (CompareString(Word, "print") == 0) {
                    RHS = BreakOffWord(Word = RHS);

                    if (CompareString(Word, "top_axis") == 0) {
                        NewDocument->Properties |= DOC_PRINT_TOP;
                    }
                    else if (CompareString(Word, "side_axis") == 0) {
                        NewDocument->Properties |= DOC_PRINT_SIDE;
                    }
                    else if (CompareString(Word, "width") == 0) {
                        NewDocument->Properties |= DOC_PRINT_WIDTH;
                    }
                    else if (CompareString(Word, "head_sep") == 0) {
                        NewDocument->Properties |= DOC_PRINT_HEAD_SEP;
                    }
                }
                else {
                    Error("Unknown command :%s", Word);
                }
            }
        }
        else {
            char *String;
            row *Row = GetNewRow(NewDocument);

            while (*RHS) {
                RHS = BreakOffCell(String = RHS);

                cell *Cell = GetNewCell(Row);

                if (StringSize(String) < ArrayCount(Cell->Value)) {
                    if (IsEvalChar(String[0])) {
                        Cell->Status |= CELL_FUNCTION;
                    }
                }
                else {
                    Cell->Status |= CELL_ERROR;
                    String = "E:long";
                }

                BufferString(Cell->Value, ArrayCount(Cell->Value), String);
            }
        }
    }

    close(FileHandle);
    return NewDocument;
}

void FreeDocument(document *Doc) {
    for (int i = 0; i < Doc->RowCount; ++i) {
        row *Row = Doc->Row + i;

        free(Row->Cell);
    }

    if (Doc->DirFD != AT_FDCWD) close(Doc->DirFD);
    free(Doc->Row);
    free(Doc);
}

row *GetNewRow(document *Root) {
    row *Row = NULL;

    if (Root) {
        if (Root->RowCount >= Root->RowCap) {
            Root->RowCap *= 2;
            Root->Row = realloc(Root->Row, sizeof *Root->Row * Root->RowCap);
        }

        Row = Root->Row + Root->RowCount++;
        Row->CellCap = 1;
        Row->CellCount = 0;
        Row->Cell = malloc(sizeof *Row->Cell * Row->CellCap);
    }

    return Row;
}

static inline
cell *GetNewCellPreChecked(row *Row) {
    if (Row->CellCount >= Row->CellCap) {
        Row->CellCap *= 2;
        Row->Cell = realloc(Row->Cell, sizeof *Row->Cell * Row->CellCap);
    }

    cell *Cell = Row->Cell + Row->CellCount++;

    return Cell;
}

cell *GetNewCell(row *Root) {
    cell *Cell = NULL;

    if (Root) {
        Cell = GetNewCellPreChecked(Root);
        Cell->Status = 0;
        Cell->Width = DEFAULT_CELL_WIDTH;
    }

    return Cell;
}

void MemSet(void *Destination, size_t Size, char Byte) {
    char *End = (char *)Destination + Size;

    char *Cur = Destination;

    while (Cur < End) {
        *Cur++ = Byte;
    }
}

void MemCopy(void *Destination, size_t Size, void *Source) {
    char *End = (char *)Destination + Size;

    char *DCur = Destination;
    char *SCur = Source;

    while (DCur < End) {
        *DCur++ = *SCur++;
    }
}
