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
void PrintRow(document *Doc, int r, int Margin) {
    int c;
    column *Column;
    char *Value;
    static char EmptyString[1] = "";

    if (Doc->Properties & DOC_PRINT_SIDE) {
        printf("%*d  ", Margin, r+1);
    }

    for (c = 0; c < Doc->ColumnCount - 1; ++c) {
        Value = EmptyString;
        Column = Doc->Column + c;

        if (r < Column->CellCount) {
            Value = EvaluateCell(Doc, Column->Cell + r);
        }

        PrintStringCell(Value, INNER_FS, Column->Width);
    }

    if (c < Doc->ColumnCount) {
        Value = EmptyString;

        Column = Doc->Column + c;
        if (r < Column->CellCount) {
            Value = EvaluateCell(Doc, Column->Cell + r);
        }

        PrintStringCell(Value, OUTER_FS, Column->Width);
    }
}

static inline
void PrintHeadRow(document *Doc, int i, int Margin) {
    int j;
    int Width;
    static char Sep[MAX_COLUMN_WIDTH+1];

    PrintRow(Doc, i, Margin);

    if (Doc->Properties & DOC_PRINT_SIDE) {
        printf("%*s  ", Margin, "");
    }

    for (j = 0; j < Doc->ColumnCount - 1; ++j) {
        Width = Doc->Column[j].Width;

        for (int c = 0; c < Width; ++c) Sep[c] = '-';
        Sep[Width+1] = 0;

        PrintStringCell(Sep, INNER_FS, Width);
    }

    if (j < Doc->ColumnCount) {
        Width = Doc->Column[j].Width;

        for (int c = 0; c < Width; ++c) Sep[c] = '-';
        Sep[Width+1] = 0;

        PrintStringCell(Sep, OUTER_FS, Width);
    }
}

static inline
void PrintTopRuler(document *Doc, int Margin) {
    int j;
    char Name[2] = "A";

    /* @TEMP: for now there can be now more than 26 columns. */
    if (Doc->Properties & DOC_PRINT_SIDE) {
        printf("%*s  ", Margin, "");
    }

    for (j = 0; j < Doc->ColumnCount - 1; ++j) {
        PrintStringCell(Name, INNER_FS, Doc->Column[j].Width);
        ++Name[0];
    }

    PrintStringCell(Name, OUTER_FS, Doc->Column[j].Width);
}

static inline
void PrintColumnWidths(document *Doc, int Margin) {
    int Width;
    int i;

    if (Doc->Properties & DOC_PRINT_SIDE) {
        printf("%*s  ", Margin, "");
    }

    for (i = 0; i < Doc->ColumnCount - 1; ++i) {
        Width = Doc->Column[i].Width;
        PrintNumCell(Width, INNER_FS, Width);
    }

    Width = Doc->Column[i].Width;
    PrintNumCell(Width, OUTER_FS, Width);
}

int main(int argc, char **argv) {
    document *Doc = NULL;

    /* TODO: true argument parsing */
    if (argc == 2) {
        char *FileName = argv[1];
        Doc = ReadDocument(FileName);
        if (Doc) {
            int MaxColumnWidth = 0;
            int MaxRowCount = 0;

            for (int i = 0; i < Doc->ColumnCount; ++i) {
                int ColumnWidth = Doc->Column[i].Width;
                int RowCount = Doc->Column[i].CellCount;

                if (ColumnWidth > MaxColumnWidth) {
                    MaxColumnWidth = ColumnWidth;
                }

                if (RowCount > MaxRowCount) {
                    MaxRowCount = RowCount;
                }
            }

            int Margin = DecimalWidth(MaxColumnWidth);

            if (Doc->Properties & DOC_PRINT_WIDTH) {
                PrintColumnWidths(Doc, Margin);
            }

            if (Doc->Properties & DOC_PRINT_TOP) {
                PrintTopRuler(Doc, Margin);
            }

            if (MaxRowCount > 0) {
                if (Doc->Properties & DOC_PRINT_HEAD_SEP) {
                    PrintHeadRow(Doc, 0, Margin);
                }
                else {
                    PrintRow(Doc, 0, Margin);
                }
            }

            for (int i = 1; i < MaxRowCount; ++i) {
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
