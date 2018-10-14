#include <main.h>
#include <sc_mem.h>
#include <dbg.h> /* TODO: is Zed Shaw's dbg.h good? */

#include <sc_strings.h>

#include <stdlib.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

static inline
document *AllocDocument() {
    document *Document = malloc(sizeof *Document);

#   define INITIAL_ROW_CAP 1

    *Document = (document){
        .RowCap = INITIAL_ROW_CAP,
        .RowCount = 0,
        .Row = malloc(sizeof *Document->Row * INITIAL_ROW_CAP),
        .PrintTopAxis = false,
        .PrintSideAxis = false,
    };

    for (size_t i = 0; i < ArrayCount(Document->ColWidth); ++i) {
        Document->ColWidth[i] = 8;
    }

    return Document;
}

document *ReadSheetAt(int DirFD, char *FileName) {
    char Buffer[1024];
    int FD;
    FILE *File;
    document *Sheet;

    if ((FD = openat(DirFD, FileName, O_RDONLY)) < 0) {
        return NULL;
    }

    if (!(File = fdopen(FD, "r"))) {
        close(FD);
        return NULL;
    }

    Sheet = AllocDocument();

    /* TODO: is Buffer big enough? */
    BufferString(Buffer, ArrayCount(Buffer), FileName);
    if (BreakAtLastChar(Buffer, '/') && CompareString(Buffer, ".") != 0) {
        if (*Buffer) {
            Sheet->DirFD = open(Buffer, O_DIRECTORY | O_RDONLY);
        }
        else {
            Sheet->DirFD = open("/", O_DIRECTORY | O_RDONLY);
        }
    }
    else {
        Sheet->DirFD = DirFD;
    }

    while (GetLine(Buffer, ArrayCount(Buffer), File) > 0) {
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
                        RHS = BreakOffWord(Word = RHS);

                        /* TODO: check for errors from strtol */
                        Sheet->ColWidth[Column++] = strtol(Word, NULL, 0);
                    }
                }
                else if (CompareString(Word, "print") == 0) {
                    RHS = BreakOffWord(Word = RHS);

                    if (CompareString(Word, "top_axis") == 0) {
                        Sheet->PrintTopAxis = true;
                    }
                    else if (CompareString(Word, "side_axis") == 0) {
                        Sheet->PrintSideAxis = true;
                    }
                    else if (CompareString(Word, "width") == 0) {
                        size_t Count = ArrayCount(Sheet->ColWidth);
                        for (size_t i = 0; i < Count; ++i) {
                            int Width = Sheet->ColWidth[i];
                            PrintNumCell(Width, DelimFor(Sheet, i), Width);
                        }
                    }
                }
                else {
                    Error("Unknown command :%s", Word);
                }
            }
        }
        else {
            char *String;
            row *Row = GetNewRow(Sheet);

            while (*RHS) {
                RHS = BreakOffCell(String = RHS);

                cell *Cell = GetNewCell(Row);

                if (IsEvalChar(String[0])) {
                    Cell->Status |= CELL_FUNCTION;
                }

                BufferString(Cell->Value, ArrayCount(Cell->Value), String);
            }
        }
    }

    fclose(File);
    return Sheet;
}

void FreeDocument(document *Document) {
    for (int i = 0; i < Document->RowCount; ++i) {
        row *Row = Document->Row + i;

        free(Row->Cell);
    }

    close(Document->DirFD);
    free(Document->Row);
    free(Document);
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
