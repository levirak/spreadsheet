#include <main.h>
#include <dbg.h> /* TODO: is Zed Shaw's dbg.h good? */

#include <sc_strings.h>
#include <sc_mem.h>
#include <sc_eval.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* basically a sparse lookup table */
int DecimalWidth(unsigned int Num) {
    int Width; // Maximum width Num could take

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

int main(int argc, char **argv) {
    document *Sheet = NULL;

    /* TODO: true argument parsing */
    Check(argc == 2, "USAGE: %s FILE", argv[0]);

    char *FileName = argv[1];
    Check(Sheet = ReadSheet(FileName), "failed to open file");

    int Margin = DecimalWidth(Sheet->RowCount);

    if (Sheet->PrintTopAxis) {
        if (Sheet->PrintSideAxis) {
            printf("%*s  ", Margin, "");
        }

        for (size_t i = 0; i < ArrayCount(Sheet->ColWidth); ++i) {
            char Name[2] = { 'A'+i, 0 };
            PrintStringCell(Name, DelimFor(Sheet, i), Sheet->ColWidth[i]);
        }
    }

    for (int i = 0; i < Sheet->RowCount; ++i) {
        row *Row = Sheet->Row + i;
        cell *Cell;
        int j;

        if (Sheet->PrintSideAxis) {
            printf("%*d  ", Margin, i+1);
        }

        for (j = 0; j < Row->CellCount - 1; ++j) {
            Cell = Row->Cell + j;
            Cell->Width = Sheet->ColWidth[j];

            PrintStringCell(EvaluateCell(Sheet, Cell), " ", Cell->Width);
        }

        Cell = Row->Cell + j;
        Cell->Width = Sheet->ColWidth[j];
        PrintStringCell(EvaluateCell(Sheet, Cell), "\n", Cell->Width);
    }

    FreeDocument(Sheet);

    return 0;

error:
    if (Sheet) FreeDocument(Sheet);

    return 1;
}
