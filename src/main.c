#include <main.h>

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
    if (Doc->Properties & DOC_PRINT_SIDE) {
        printf("%*d  ", Margin, r+1);
    }

    for (int c = 0; c < Doc->ColumnCount; ++c) {
        char *FS = c == Doc->ColumnCount - 1? OUTER_FS: INNER_FS;
        column *Column = Doc->Column + c;

        if (r < Column->CellCount) {
            cell *Cell = Column->Cell + r;
            EvaluateCell(Doc, Cell);
            PrintCell(Doc, Cell, FS, Column->Width, Column->Align);
        }
        else {
            printf("%*s%s", Column->Width, "", FS);
        }
    }
}

static inline
void PrintHeadRow(document *Doc, int Row, int Margin) {
    PrintRow(Doc, Row, Margin);

    if (Doc->Properties & DOC_PRINT_SIDE) {
        printf("%*s  ", Margin, "");
    }

    for (int i = 0; i < Doc->ColumnCount; ++i) {
        static char Sep[MAX_COLUMN_WIDTH+1];

        char *FS = i == Doc->ColumnCount - 1? OUTER_FS: INNER_FS;
        int Width = Doc->Column[i].Width;

        for (int c = 0; c < Width; ++c) {
            /* TODO: simplify logic? Remove looped conditional? */
            if ((c == 0 && Doc->Column[i].Align == ALIGN_LEFT)
            ||  (c == Width - 1 && Doc->Column[i].Align == ALIGN_RIGHT)) {
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

static inline
void PrintTopRuler(document *Doc, int Margin) {
    char Name[2] = "A";

    /* @TEMP: for now there can be now more than 26 columns. */
    if (Doc->Properties & DOC_PRINT_SIDE) {
        printf("%*s" MARGIN_FS, Margin, "");
    }

    for (int i = 0; i < Doc->ColumnCount; ++i) {
        char *FS = i == Doc->ColumnCount - 1? OUTER_FS: INNER_FS;
        printf("%-*s%s", Doc->Column[i].Width, Name, FS);
        ++Name[0];
    }
}

static inline
void PrintColumnWidths(document *Doc, int Margin) {
    if (Doc->Properties & DOC_PRINT_SIDE) {
        printf("%*s  ", Margin, "");
    }

    for (int i = 0; i < Doc->ColumnCount; ++i) {
        char *FS = i == Doc->ColumnCount - 1? OUTER_FS: INNER_FS;
        int Width = Doc->Column[i].Width;
        PrintNumber(Width, FS, Width, 0);
    }
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
            printf("Failed to open %s\n", FileName);
            return 1;
        }
    }
    else {
        printf("USAGE: %s FILE\n", argv[0]);
        return 1;
    }
}
