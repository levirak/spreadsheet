#include "main.h"

#include "strings.h"
#include "mem.h"
#include "eval.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

/* basically a sparse lookup table */
static inline u32
DecimalWidth(u32 Num)
{
    u32 Width;

    /* This should cover a 32 bit integer. Add more if neccesary */
    if      (Num < 10)          Width =  1;
    else if (Num < 100)         Width =  2;
    else if (Num < 1000)        Width =  3;
    else if (Num < 10000)       Width =  4;
    else if (Num < 100000)      Width =  5;
    else if (Num < 1000000)     Width =  6;
    else if (Num < 10000000)    Width =  7;
    else if (Num < 100000000)   Width =  8;
    else if (Num < 1000000000)  Width =  9;
    else                        Width = 10;

    return Width;
}

static inline void
PrintRow(document *Doc, s32 r, s32 Margin)
{
    if (Doc->Properties & DOC_PRINT_SIDE) {
        printf("%*d  ", Margin, r+1);
    }

    for (s32 c = 0; c < Doc->ColumnCount; ++c) {
        char *FS = c == Doc->ColumnCount - 1? OUTER_FS: INNER_FS;
        column *Column = Doc->Column + c;

        cell *Cell = 0;

        if (r < Column->CellCount) {
            Cell = Column->Cell + r;
            EvaluateCell(Doc, Cell);
        }

        PrintCell(Cell, FS, Column->Width, Column->Align);
    }
}

static inline void
PrintHeadRow(document *Doc, u32 Row, u32 Margin)
{
    PrintRow(Doc, Row, Margin);

    if (Doc->Properties & DOC_PRINT_SIDE) {
        printf("%*s  ", Margin, "");
    }

    for (s32 Idx = 0; Idx < Doc->ColumnCount; ++Idx) {
        static char Sep[MAX_COLUMN_WIDTH+1];

        char *FS = Idx == Doc->ColumnCount - 1? OUTER_FS: INNER_FS;
        s32 Width = Doc->Column[Idx].Width;

        for (s32 c = 0; c < Width; ++c) {
            /* TODO: simplify logic? Remove looped conditional? */
            if ((c == 0 && Doc->Column[Idx].Align == ALIGN_LEFT)
            ||  (c == Width - 1 && Doc->Column[Idx].Align == ALIGN_RIGHT)) {
                Sep[c] = ':';
            }
            else {
                Sep[c] = '-';
            }
        }

        Sep[Width] = '\0';

        printf("%s%s", Sep, FS);
    }
}

static inline void
PrintTopRuler(document *Doc, s32 Margin)
{
    char Name[2] = "A";

    /* @TEMP: for now there can be now more than 26 columns. */
    if (Doc->Properties & DOC_PRINT_SIDE) {
        printf("%*s" MARGIN_FS, Margin, "");
    }

    for (s32 Idx = 0; Idx < Doc->ColumnCount; ++Idx) {
        char *FS = Idx == Doc->ColumnCount - 1? OUTER_FS: INNER_FS;
        printf("%-*s%s", Doc->Column[Idx].Width, Name, FS);
        ++Name[0];
    }
}

static inline void
PrintColumnWidths(document *Doc, s32 Margin)
{
    if (Doc->Properties & DOC_PRINT_SIDE) {
        printf("%*s  ", Margin, "");
    }

    for (s32 Idx = 0; Idx < Doc->ColumnCount; ++Idx) {
        char *FS = Idx == Doc->ColumnCount - 1? OUTER_FS: INNER_FS;
        s32 Width = Doc->Column[Idx].Width;
        PrintNumber(Width, FS, Width, 0);
    }
}

static inline s32
EvalAndPrintSpreadsheet(char *FileName)
{
    document *Doc = ReadDocument(FileName);

    if (Doc) {
        s32 MaxColumnWidth = 0;
        s32 MaxRowCount = 0;

        for (s32 Idx = 0; Idx < Doc->ColumnCount; ++Idx) {
            s32 ColumnWidth = Doc->Column[Idx].Width;
            s32 RowCount = Doc->Column[Idx].CellCount;

            if (ColumnWidth > MaxColumnWidth) {
                MaxColumnWidth = ColumnWidth;
            }

            if (RowCount > MaxRowCount) {
                MaxRowCount = RowCount;
            }
        }

        s32 Margin = DecimalWidth(MaxColumnWidth);

        if (Doc->Properties & DOC_PRINT_WIDTH) {
            PrintColumnWidths(Doc, Margin);
        }

        if (Doc->Properties & DOC_PRINT_TOP) {
            PrintTopRuler(Doc, Margin);
        }

        for (s32 Idx = 0; Idx < MaxRowCount; ++Idx) {
            if (Doc->Properties & DOC_PRINT_HEAD_SEP && Idx == Doc->HeadSepIdx) {
                PrintHeadRow(Doc, Idx, Margin);
            }
            else {
                PrintRow(Doc, Idx, Margin);
            }
        }

        FreeDocument(Doc);

        return 0;
    }
    else {
        printf("Failed to open %s\n", FileName);
        return 1;
    }
}

s32 main(s32 ArgCount, char **ArgVec)
{
    bool PrintFilePaths = ArgCount > 2;
    s32 ReturnCode = 0;

    /* NOTE: this call asks glibc to set all the locale from the environment */
    setlocale(LC_ALL, "");

    /* TODO: true argument parsing */

    if (ArgCount < 2) {
        printf("USAGE: %s FILE...\n", ArgVec[0]);
        ReturnCode = 1;
    }
    else for (s32 Idx = 1;;) {
        char *FileName = ArgVec[Idx++];

        if (PrintFilePaths) {
            printf("%s:\n", FileName);
        }

        /* TODO: better handling of aggregated return codes */
        ReturnCode = EvalAndPrintSpreadsheet(FileName);

        if (ReturnCode || Idx >= ArgCount) break;

        printf("\n");
    }


    return ReturnCode;
}
