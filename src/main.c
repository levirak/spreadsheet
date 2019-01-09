#include <main.h>
#include <dbg.h> /* TODO: is Zed Shaw's dbg.h good? */

#include <sc_strings.h>
#include <sc_mem.h>
#include <sc_eval.h>

#include <stdio.h>
#include <stdlib.h>

/* basically a sparse lookup table */
static inline
int DecimalWidth(unsigned int Num) {
    int Width;

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

static inline
void PrintRow(document *Doc, int i, int Margin) {
    row *Row = Doc->Row + i;
    cell *Cell;
    int j = 0;

    if (Doc->Properties & DOC_PRINT_SIDE) {
        printf("%*d  ", Margin, i+1);
    }

    for (j = 0; j < Row->CellCount - 1; ++j) {
        Cell = Row->Cell + j;
        Cell->Width = Doc->ColWidth[j];
        PrintStringCell(EvaluateCell(Doc, Cell), INNER_FS, Cell->Width);
    }

    if (j < Row->CellCount) {
        Cell = Row->Cell + j;
        Cell->Width = Doc->ColWidth[j];
        PrintStringCell(EvaluateCell(Doc, Cell), OUTER_FS, Cell->Width);
    }
}

static inline
void PrintHeadRow(document *Doc, int i, int Margin) {
    int j;
    int Width;
    static char Sep[MAX_CELL_WIDTH+1];
    int ColCount = Doc->Row[i].CellCount;

    PrintRow(Doc, i, Margin);

    if (Doc->Properties & DOC_PRINT_SIDE) {
        printf("%*s  ", Margin, "");
    }

    for (j = 0; j < ColCount - 1; ++j) {
        Width = Doc->ColWidth[j];

        for (int c = 0; c < Width; ++c) Sep[c] = '-';
        Sep[Width+1] = 0;

        PrintStringCell(Sep, INNER_FS, Width);
    }

    if (j < ColCount) {
        Width = Doc->ColWidth[j];

        for (int c = 0; c < Width; ++c) Sep[c] = '-';
        Sep[Width+1] = 0;

        PrintStringCell(Sep, OUTER_FS, Width);
    }
}

static inline
void PrintTopRuler(document *Doc, int Margin) {
    int j;
    char Name[2] = "A";

    if (Doc->Properties & DOC_PRINT_SIDE) {
        printf("%*s  ", Margin, "");
    }

    for (j = 0; j < (int)ArrayCount(Doc->ColWidth) - 1; ++j) {
        PrintStringCell(Name, INNER_FS, Doc->ColWidth[j]);
        ++Name[0];
    }

    PrintStringCell(Name, OUTER_FS, Doc->ColWidth[j]);
}

static inline
void PrintColumnWidths(document *Doc, int Margin) {
    int Width;
    int i;

    if (Doc->Properties & DOC_PRINT_SIDE) {
        printf("%*s  ", Margin, "");
    }

    for (i = 0; i < (int)ArrayCount(Doc->ColWidth) - 1; ++i) {
        Width = Doc->ColWidth[i];
        PrintNumCell(Width, INNER_FS, Width);
    }

    Width = Doc->ColWidth[i];
    PrintNumCell(Width, OUTER_FS, Width);
}

int main(int argc, char **argv) {
    document *Doc = NULL;

    /* TODO: true argument parsing */
    if (argc == 2) {
        char *FileName = argv[1];
        Doc = ReadDocument(FileName);
        if (Doc) {
            int Margin = DecimalWidth(Doc->RowCount);

            if (Doc->Properties & DOC_PRINT_WIDTH) {
                PrintColumnWidths(Doc, Margin);
            }

            if (Doc->Properties & DOC_PRINT_TOP) {
                PrintTopRuler(Doc, Margin);
            }

            if (Doc->RowCount > 0) {
                PrintHeadRow(Doc, 0, Margin);
            }

            for (int i = 1; i < Doc->RowCount; ++i) {
                PrintRow(Doc, i, Margin);
            }

            FreeDocument(Doc);

            return 0;
        }
        else {
        printf("failed to open file");
        return 1;
        }
    }
    else {
        printf("USAGE: %s FILE", argv[0]);
        return 1;
    }
}
