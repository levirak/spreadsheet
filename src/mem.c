#include "main.h"
#include "mem.h"

#include "strings.h"
#include "eval.h"

#include <stdlib.h>
#include <stdint.h>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef struct page
{
    mm Size;
    mm Used;
    struct page *Next;
    char Data[0];
} page;

#define PAGE_ALLOC_SIZE (512ul)
static_assert(PAGE_ALLOC_SIZE > sizeof (page), "Allocation size too small.");

static inline
document *AllocDocument()
{
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

static struct
{
    u32 Size;
    u32 Used;
    struct doc_cache_entry {
        ino_t Inode;
        document *Doc;
        s32 Num;
    } *Data;
static_assert (sizeof (ino_t) == 8);
static_assert (sizeof (struct doc_cache_entry) == 24);
} DocCache = { 0 };

static inline
ino_t GetInode(fd DirFD, char *FileName)
{
    struct stat Stat;
    ino_t Inode = 0; /* TODO(lrak): is 0 always invalid? */

    if (fstatat(DirFD, FileName, &Stat, 0) == 0) {
        Inode = Stat.st_ino;
    }

    return Inode;
}

static inline
s32 GetCacheIdxByInode(ino_t Inode)
{
    s32 Result = -1;

    for (u32 Idx = 0; Idx < DocCache.Used; ++Idx) {
        if (DocCache.Data[Idx].Inode == Inode) {
            Result = Idx;
            break;
        }
    }

    if (Result == -1) {
        if (DocCache.Used >= DocCache.Size) {
            DocCache.Size = DocCache.Size ? 2*DocCache.Size : 32;
            DocCache.Data = realloc(DocCache.Data,
                                    DocCache.Size * sizeof *DocCache.Data);

            Assert(DocCache.Size > DocCache.Used);

            memset(DocCache.Data + DocCache.Used, 0,
                   (DocCache.Size - DocCache.Used) * sizeof *DocCache.Data);
        }

        Result = DocCache.Used++;
    }

    return Result;
}

document *ReadDocumentRelativeTo(document *Doc, char *FileName)
{
    char Buffer[256];
    document *NewDoc = 0;

    fd DirFD = Doc? Doc->DirFD: AT_FDCWD;

    ino_t Inode = GetInode(DirFD, FileName);
    s32 Idx;

    if (!Inode) {
        /* no-op */
    }
    else if ((Idx = GetCacheIdxByInode(Inode)) < 0) {
        fprintf(stderr, "! no index for %s\n", FileName);
        NotImplemented;
    }
    else if (DocCache.Data[Idx].Doc) {
        Assert(DocCache.Data[Idx].Inode == Inode);

        NewDoc = DocCache.Data[Idx].Doc;
        ++DocCache.Data[Idx].Num;
    }
    else {
        fd FileHandle = openat(DirFD, FileName, O_RDONLY);
        Assert(FileHandle >= 0);

        NewDoc = AllocDocument();

        /* TODO: is Buffer big enough? */
        BufferString(Buffer, ArrayCount(Buffer), FileName);
        if (BreakAtLastChar(Buffer, '/') && CompareString(Buffer, ".") != 0) {
            if (*Buffer) {
                NewDoc->DirFD = openat(DirFD, Buffer, O_DIRECTORY | O_RDONLY);
            }
            else {
                NewDoc->DirFD = openat(DirFD, "/", O_DIRECTORY | O_RDONLY);
            }
        }
        else if (DirFD == AT_FDCWD) {
            NewDoc->DirFD = AT_FDCWD;
        }
        else {
            NewDoc->DirFD = dup(DirFD);
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

                            GetColumn(NewDoc, ColumnIndex++)->Width = Width;
                        }
                    }
                    else if (CompareString(Word, "print") == 0) {
                        RHS = BreakOffWord(Word = RHS);

                        if (CompareString(Word, "top_axis") == 0) {
                            NewDoc->Properties |= DOC_PRINT_TOP;
                        }
                        else if (CompareString(Word, "side_axis") == 0) {
                            NewDoc->Properties |= DOC_PRINT_SIDE;
                        }
                        else if (CompareString(Word, "width") == 0) {
                            NewDoc->Properties |= DOC_PRINT_WIDTH;
                        }
                        else if (CompareString(Word, "head_sep") == 0) {
                            NewDoc->Properties |= DOC_PRINT_HEAD_SEP;

                            RHS = BreakOffWord(Word = RHS);

                            if (*Word) {
                                char *Trailing;
                                s32 Row = StringToInt(Word, &Trailing);

                                if (*Trailing) {
                                    PrintError("Trailing after width: %s", Word);
                                }

                                NewDoc->HeadSepIdx = Row - 1;
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

                            GetColumn(NewDoc, ColumnIndex++)->Align = Align;
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

                    if (*String) {
                        cell *Cell = GetCell(NewDoc, ColumnIndex, RowIndex);

                        if (String[0] == '"') {
                            /* string literal */
                            ++String;

                            /* TODO(lrak): deal with escaped '"' and '\t' */
                            char *Cur = String;
                            while (*Cur && *Cur != '"') ++Cur;
                            *Cur = 0;

                            Cell->Value = (cell_value){
                                .Type = CELL_TYPE_STRING,
                                .AsString = PushString(NewDoc, String),
                            };
                        }
                        else if (LooksLikeInt(String)) {
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
                            Cell->Value =
                                Compile(NewDoc, String + 1, &Cell->ErrorCode);
                        }
                        else {
                            Cell->Value = (cell_value){
                                .Type = CELL_TYPE_STRING,
                                .AsString = PushString(NewDoc, String),
                            };
                        }
                    }

                    ++ColumnIndex;
                }

                ++RowIndex;
            }
        }

        close(FileHandle);

        DocCache.Data[Idx] = (struct doc_cache_entry){ Inode, NewDoc, 1 };
    }

    return NewDoc;
}

void FreeDocument(document *Doc)
{
    s32 NumOpen = 0;

    for (u32 Idx = 0; Idx < DocCache.Used; ++Idx) {
        if (DocCache.Data[Idx].Doc == Doc) {
            --DocCache.Data[Idx].Num;
        }

        NumOpen += DocCache.Data[Idx].Num;
    }

    if (NumOpen == 0) {
        for (u32 Idx = 0; Idx < DocCache.Used; ++Idx) {
            Assert(DocCache.Data[Idx].Doc);
            Assert(DocCache.Data[Idx].Num == 0);

            document *CachedDoc = DocCache.Data[Idx].Doc;

            for (s32 i = 0; i < CachedDoc->ColumnCount; ++i) {
                column *Column = CachedDoc->Column + i;

                free(Column->Cell);
            }

            if (CachedDoc->DirFD != AT_FDCWD) close(CachedDoc->DirFD);
            free(CachedDoc->Column);

            for (page *Next, *Cur = CachedDoc->Strings; Cur; Cur = Next) {
                Next = Cur->Next;
                free(Cur);
            }

            free(CachedDoc);
        }

        free(DocCache.Data);
    }
}

column *GetColumn(document *Doc, s32 ColumnIndex)
{
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

cell *GetCell(document *Doc, s32 ColumnIndex, s32 RowIndex)
{
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

void MemSet(void *Destination, mm Size, char Byte)
{
    char *End = (char *)Destination + Size;

    char *Cur = Destination;

    while (Cur < End) {
        *Cur++ = Byte;
    }
}

void MemCopy(void *Destination, mm Size, void *Source)
{
    char *End = (char *)Destination + Size;

    char *DCur = Destination;
    char *SCur = Source;

    while (DCur < End) {
        *DCur++ = *SCur++;
    }
}

void *PushSize(document *Doc, mm Size, mm Align)
{
    Assert(Doc);
    Assert(Align);

    mm Used(page *Page) {
        Assert(Page);
        mm Bumped = Page->Used + (Align - 1);
        return Bumped - (Bumped % Align);
    }

    mm NumPages = 0;

    page **Cur = &Doc->Strings;
    while (*Cur && (**Cur).Size <= Used(*Cur) + Size) {
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
        Assert(Size <= NewPage->Size);

        *Cur = NewPage;

        Assert((char *)(*Cur) + NewSize == (**Cur).Data + (**Cur).Size);
    }

    page *Page = *Cur;

    char *AllocatedRange = Page->Data + Used(Page);
    Page->Used = Used(Page) + Size;

    return AllocatedRange;
}

char *PushString(document *Doc, char *InString)
{
    Assert(Doc);
    Assert(InString);

    mm Size = StringLength(InString) + 1;
    char *Buffer = PushSize(Doc, Size, 1);

    CheckEq(BufferString(Buffer, Size, InString), Size - 1);
    return Buffer;
}
