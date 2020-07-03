#include <main.h>
#include <sc_mem.h>

#include <sc_strings.h>

#include <stdlib.h>
#include <stdint.h>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef struct page {
    mm Size;
    mm Used;
    struct page *Next;
    char Data[0];
} page;

#define PAGE_ALLOC_SIZE (512ul)
static_assert(PAGE_ALLOC_SIZE > sizeof (page), "Allocation size too small.");

static inline
document *AllocDocument() {
    document *Doc = malloc(sizeof *Doc);

#define INITIAL_COLUMN_CAP 4
    *Doc = (document){
        .ColumnCap = INITIAL_COLUMN_CAP,
        .ColumnCount = 0,
        .Column = malloc(sizeof *Doc->Column * INITIAL_COLUMN_CAP),

        .Strings = 0, /* NOTE: will be initialized later */
    };

    return Doc;
}

document *ReadDocumentRelativeTo(document *Doc, char *FileName) {
    char Buffer[256];
    document *NewDocument;

    s32 DirFD = Doc? Doc->DirFD: AT_FDCWD;

    s32 FileHandle = openat(DirFD, FileName, O_RDONLY);
    if (FileHandle < 0) {
        return NULL;
    }

    NewDocument = AllocDocument();

    /* TODO: is Buffer big enough? */
    BufferString(Buffer, ArrayCount(Buffer), FileName);
    if (BreakAtLastChar(Buffer, '/') && CompareString(Buffer, ".") != 0) {
        if (*Buffer) {
            NewDocument->DirFD = openat(DirFD, Buffer, O_DIRECTORY | O_RDONLY);
        }
        else {
            NewDocument->DirFD = openat(DirFD, "/", O_DIRECTORY | O_RDONLY);
        }
    }
    else if (DirFD == AT_FDCWD) {
        NewDocument->DirFD = AT_FDCWD;
    }
    else {
        NewDocument->DirFD = dup(DirFD);
    }

    s32 RowIndex = 0;
    while (GetLine(Buffer, ArrayCount(Buffer), FileHandle) >= 0) {
        s32 ColumnIndex = 0;
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
                        s32 Width = StringToInt(Word, &Trailing);

                        if (Width > MAX_COLUMN_WIDTH) {
                            PrintError("Truncating overwide cell: %d (max %d)",
                                  Width, MAX_COLUMN_WIDTH);
                            Width = MAX_COLUMN_WIDTH;
                        }

                        if (*Trailing) {
                            PrintError("Trailing after width: %s", Word);
                        }

                        GetColumn(NewDocument, ColumnIndex++)->Width = Width;
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

                        RHS = BreakOffWord(Word = RHS);

                        if (*Word) {
                            char *Trailing;
                            s32 Row = StringToInt(Word, &Trailing);

                            if (*Trailing) {
                                PrintError("Trailing after width: %s", Word);
                            }

                            NewDocument->HeadSepIdx = Row - 1;
                        }
                    }
                }
                else if (CompareString(Word, "align") == 0) {
                    s32 Align = 0;
                    while (*RHS) {
                        RHS = BreakOffWord(Word = RHS);

                        switch (Word[0]) {
                        case 'l':
                            Align = ALIGN_LEFT;
                            break;
                        case 'c':
                            Align = ALIGN_CENTER;
                            break;
                        case 'r':
                            Align = ALIGN_RIGHT;
                            break;
                        default:
                            PrintError("Unknown alignment specifier '%c'",
                                       Word[0]);
                            break;
                        }

                        GetColumn(NewDocument, ColumnIndex++)->Align = Align;
                    }
                }
                else {
                    PrintError("Unknown command :%s", Word);
                }
            }
        }
        else {
            char *String;

            while (*RHS) {
                RHS = BreakOffCell(String = RHS);

                cell *Cell = GetCell(NewDocument, ColumnIndex++, RowIndex);

                if (*String) {
                    if (LooksLikeInt(String)) {
                        Cell->Value = (cell_value){
                            .Type = CELL_TYPE_INT,
                            .AsInt = StringToInt(String, 0),
                        };
                    }
                    else if (LooksLikeReal(String)) {
                        Cell->Value = (cell_value){
                            .Type = CELL_TYPE_REAL,
                            .AsReal = StringToReal(String, 0),
                        };
                    }
                    else if (IsEvalChar(String[0])) {
                        Cell->Value = (cell_value){
                            .Type = CELL_TYPE_EXPR,
                            .AsExpr = PushString(NewDocument, String + 1),
                        };
                    }
                    else {
                        Cell->Value = (cell_value){
                            .Type = CELL_TYPE_STRING,
                            .AsString = PushString(NewDocument, String),
                        };
                    }
                }
            }

            ++RowIndex;
        }
    }

    close(FileHandle);
    return NewDocument;
}

void FreeDocument(document *Doc) {
    for (s32 i = 0; i < Doc->ColumnCount; ++i) {
        column *Column = Doc->Column + i;

        free(Column->Cell);
    }

    if (Doc->DirFD != AT_FDCWD) close(Doc->DirFD);
    free(Doc->Column);

    for (page *Next, *Cur = Doc->Strings; Cur; Cur = Next) {
        Next = Cur->Next;
        free(Cur);
    }

    free(Doc);
}

column *GetColumn(document *Doc, s32 ColumnIndex) {
    while (Doc->ColumnCount <= ColumnIndex) {
        while (Doc->ColumnCount >= Doc->ColumnCap) {
            Doc->ColumnCap *= 2;
            Doc->Column = realloc(Doc->Column,
                                  Doc->ColumnCap * sizeof *Doc->Column);
        }

        column *Column = Doc->Column + Doc->ColumnCount++;

#       define INITIAL_ROW_CAP 8

        *Column = (column){
            .Width = DEFAULT_CELL_WIDTH,
            .CellCap = INITIAL_ROW_CAP,
            .CellCount = 0,
            .Cell = malloc(sizeof *Column->Cell * INITIAL_ROW_CAP),
        };
    }

    return Doc->Column + ColumnIndex;
}

cell *GetCell(document *Doc, s32 ColumnIndex, s32 RowIndex) {
    column *Column = GetColumn(Doc, ColumnIndex);

    while (Column->CellCount <= RowIndex) {
        while (Column->CellCount >= Column->CellCap) {
            Column->CellCap *= 2;
            Column->Cell = realloc(Column->Cell,
                                   Column->CellCap * sizeof *Column->Cell);
        }

        cell *Cell = Column->Cell + Column->CellCount++;

        *Cell = (cell){};
    }

    return Column->Cell + RowIndex;
}

void MemSet(void *Destination, mm Size, char Byte) {
    char *End = (char *)Destination + Size;

    char *Cur = Destination;

    while (Cur < End) {
        *Cur++ = Byte;
    }
}

void MemCopy(void *Destination, mm Size, void *Source) {
    char *End = (char *)Destination + Size;

    char *DCur = Destination;
    char *SCur = Source;

    while (DCur < End) {
        *DCur++ = *SCur++;
    }
}

char *PushString(document *Doc, char *InString) {
    Assert(Doc);
    Assert(InString);

    mm Length = StringSize(InString) + 1;
    mm NumPages = 0;

    page **Cur = &Doc->Strings;
    while (*Cur && (**Cur).Size <= (**Cur).Used + Length) {
        Assert((PAGE_ALLOC_SIZE << NumPages) <= (SIZE_MAX >> 1));

        Cur = &(**Cur).Next;
        ++NumPages;
    }

    if (!*Cur) {
        mm NewSize = PAGE_ALLOC_SIZE << NumPages;

        page *NewPage = malloc(NewSize);
        *NewPage = (page){
            .Size = NewSize - sizeof *NewPage,
            .Used = 0,
            .Next = 0,
        };
        Assert(Length <= NewPage->Size);

        *Cur = NewPage;

        Assert((char *)(*Cur) + NewSize == (**Cur).Data + (**Cur).Size);
    }

    page *Page = *Cur;

    char *OutString = Page->Data + Page->Used;
    mm Written = BufferString(OutString, Length, InString) + 1;
    Page->Used += Written;

    Assert(Written == Length);

    return OutString;
}
