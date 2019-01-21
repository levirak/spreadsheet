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

#   define INITIAL_COLUMN_CAP 1

    *Doc = (document){
        .ColumnCap = INITIAL_COLUMN_CAP,
        .ColumnCount = 0,
        .Column = malloc(sizeof *Doc->Column * INITIAL_COLUMN_CAP),

        .StringStackCap = INITIAL_STRING_STACK_SIZE,
        /* The empty string is going to be "pre-buffered" at offset 0 */
        .StringStackUsed = 1,
        .StringStack = malloc(INITIAL_STRING_STACK_SIZE),
    };

    Doc->StringStack[0] = '\0';

    return Doc;
}

document *ReadDocumentRelativeTo(document *Doc, char *FileName) {
    char Buffer[256];
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

    int RowIndex = 0;
    while (GetLine(Buffer, ArrayCount(Buffer), FileHandle) > 0) {
        int ColumnIndex = 0;
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
                    }
                }
                else if (CompareString(Word, "align") == 0) {
                    int Align = 0;
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
                            Error("Unknown alignment specifier '%c'", Word[0]);
                            break;
                        }

                        GetColumn(NewDocument, ColumnIndex++)->Align = Align;
                    }
                }
                else {
                    Error("Unknown command :%s", Word);
                }
            }
        }
        else {
            char *String;

            while (*RHS) {
                RHS = BreakOffCell(String = RHS);

                cell *Cell = GetCell(NewDocument, ColumnIndex++, RowIndex);

                if (IsEvalChar(String[0])) {
                    Cell->Status |= CELL_FUNCTION;
                }

                Cell->Offset = PushString(NewDocument, String);
            }

            ++RowIndex;
        }
    }

    close(FileHandle);
    return NewDocument;
}

void FreeDocument(document *Doc) {
    for (int i = 0; i < Doc->ColumnCount; ++i) {
        column *Column = Doc->Column + i;

        free(Column->Cell);
    }

    if (Doc->DirFD != AT_FDCWD) close(Doc->DirFD);
    free(Doc->Column);
    free(Doc->StringStack);
    free(Doc);
}

column *GetColumn(document *Doc, int ColumnIndex) {
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

cell *GetCell(document *Doc, int ColumnIndex, int RowIndex) {
    column *Column = GetColumn(Doc, ColumnIndex);

    while (Column->CellCount <= RowIndex) {
        while (Column->CellCount >= Column->CellCap) {
            Column->CellCap *= 2;
            Column->Cell = realloc(Column->Cell,
                                   Column->CellCap * sizeof *Column->Cell);
        }

        cell *Cell = Column->Cell + Column->CellCount++;

        *Cell = (cell){
            .Status = 0,
            .Offset = 0,
        };
    }

    return Column->Cell + RowIndex;
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

ptrdiff_t PushString(document *Doc, char *InString) {
    size_t Length = StringSize(InString) + 1;
    size_t Offset = Doc->StringStackUsed;

    Assert(Doc->StringStackUsed <= Doc->StringStackCap);

    Doc->StringStackUsed += Length;

    if (Doc->StringStackCap < Doc->StringStackUsed) {
        do {
            /* TODO: don't loop here */
            Doc->StringStackCap += INITIAL_STRING_STACK_SIZE;
        }
        while (Doc->StringStackCap < Doc->StringStackUsed);

        Doc->StringStack = realloc(Doc->StringStack, Doc->StringStackCap);
    }

    Assert(Doc->StringStackUsed <= Doc->StringStackCap);

    char *OutString = Doc->StringStack + Offset;
    size_t Written = BufferString(OutString, Length, InString);

    Assert(Written + 1 == Length);

    return OutString - Doc->StringStack;
}
