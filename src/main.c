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
    if      (Num < 10)          return  1;
    else if (Num < 100)         return  2;
    else if (Num < 1000)        return  3;
    else if (Num < 10000)       return  4;
    else if (Num < 100000)      return  5;
    else if (Num < 1000000)     return  6;
    else if (Num < 10000000)    return  7;
    else if (Num < 100000000)   return  8;
    else if (Num < 1000000000)  return  9;
    else                        return 10;
    /* This should cover a 32 bit integer. Add more if neccesary */
}

int main(int argc, char **argv) {
    char Buffer[1024];
    FILE *File = NULL;
    document *Spreadsheet = NULL;

    bool PrintTopAxis = false;
    bool PrintSideAxis = false;

    /* @TEMP: this row count
    * Note that for now there can be now more than 26 rows.
    */
    int ColWidth[8];

    /* TODO: true argument parsing */
    Check(argc == 2, "USAGE: %s FILE", argv[0]);

    Check(File = fopen(argv[1], "r"), "failed to open file");

    /* initialize ColWidth */
    for (size_t i = 0; i < ArrayCount(ColWidth); ++i) {
        ColWidth[i] = 8;
    }

    Spreadsheet = AllocDocument();

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
                        ColWidth[Column++] = strtol(Word, NULL, 0);
                    }
                }
                else if (CompareString(Word, "print") == 0) {
                    RHS = BreakOffWord(Word = RHS);

                    if (CompareString(Word, "top_axis") == 0) {
                        PrintTopAxis = true;
                    }
                    else if (CompareString(Word, "side_axis") == 0) {
                        PrintSideAxis = true;
                    }
                    else if (CompareString(Word, "width") == 0) {
                        for (size_t i = 0; i < ArrayCount(ColWidth); ++i) {
                            int Width = ColWidth[i];
                            PrintNumCell(Width, DelimFor(i), Width);
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
            row *Row = GetNewRow(Spreadsheet);

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

    int Margin = DecimalWidth(Spreadsheet->RowCount);

    if (PrintTopAxis) {
        if (PrintSideAxis) {
            printf("%*s  ", Margin, "");
        }

        for (size_t i = 0; i < ArrayCount(ColWidth); ++i) {
            char Name[2] = { 0};
            Name[0] = 'A' + i;
            PrintStringCell(Name, DelimFor(i), ColWidth[i]);
        }
    }

    for (int i = 0; i < Spreadsheet->RowCount; ++i) {
        row *Row = Spreadsheet->Row + i;
        cell *Cell;
        int j;

        if (PrintSideAxis) {
            printf("%*d  ", Margin, i+1);
        }

        for (j = 0; j < Row->CellCount - 1; ++j) {
            Cell = Row->Cell + j;
            Cell->Width = ColWidth[j];

            PrintStringCell(EvaluateCell(Spreadsheet, Cell), " ", Cell->Width);
        }

        Cell = Row->Cell + j;
        Cell->Width = ColWidth[j];
        PrintStringCell(EvaluateCell(Spreadsheet, Cell), "\n", Cell->Width);
    }

    FreeDocument(Spreadsheet);
    fclose(File);

    return 0;

error:
    if (Spreadsheet) FreeDocument(Spreadsheet);
    if (File) fclose(File);
    return 1;
}
