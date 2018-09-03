#include <main.h>
#include <dbg.h> /* TODO: is Zed Shaw's dbg.h good? */

#include <sc_strings.h>
#include <sc_mem.h>

#include <stdio.h>
#include <stdlib.h>

enum row_type {
    ROW_NONE, /* treat as if this were ROW_HEAD */
    ROW_HEAD,
    ROW_BODY,
    ROW_FOOT,
};

int main(int argc, char **argv) {
    char Buffer[1024];
    FILE *File = NULL;
    document *Spreadsheet = NULL;
    enum row_type RowType = ROW_NONE;


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
    row *Row = GetNewRow(Spreadsheet);

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
                    if (!Row) {
                        Row = GetNewRow(Spreadsheet);
                    }

                    while (*(Word = RHS)) {
                        RHS = BreakOffWord(RHS);

                        /* TODO: check for errors from strtol */
                        ColWidth[Column++] = strtol(Word, NULL, 0);
                    }
                }
                else if (CompareString(Word, "print") == 0) {
                    Word = RHS;
                    RHS = BreakOffWord(RHS);

                    if (CompareString(Word, "top_axis") == 0) {
                        for (size_t i = 0; i < ArrayCount(ColWidth); ++i) {
                            char Name[2] = { 0};
                            Name[0] = 'A' + i;
                            PrintStringCell(Name, DelimFor(i), ColWidth[i]);
                        }
                    }
                    else if (CompareString(Word, "width") == 0) {
                        for (size_t i = 0; i < ArrayCount(ColWidth); ++i) {
                            int Width = ColWidth[i];
                            PrintNumCell(Width, DelimFor(i), Width);
                        }
                    }
                }
                else if (CompareString(Word, "begin") == 0) {
                    Word = RHS;
                    RHS = BreakOffWord(RHS);

                    if (CompareString(Word, "head") == 0) {
                        if (RowType == ROW_NONE) {
                            RowType = ROW_HEAD;
                        }
                        else if (RowType == ROW_HEAD) {
                            Error("You may only begin the header once.");
                        }
                        else {
                            Error("Head must be first.");
                        }
                    }
                    else if (CompareString(Word, "body") == 0) {
                        if (RowType == ROW_HEAD || RowType == ROW_NONE) {
                            RowType = ROW_BODY;
                        }
                        else if (RowType == ROW_BODY) {
                            Error("You may only begin the footer once.");
                        }
                        else {
                            Error("Body must come after the head.");
                        }
                    }
                    else if (CompareString(Word, "foot") == 0) {
                        if (RowType == ROW_FOOT) {
                            Error("You may only begin the footer once.");
                        }
                        else {
                            RowType = ROW_FOOT;
                        }
                    }
                    else {
                        Error("Unknown command :begin %s", Word);
                    }
                }
                else {
                    Error("Unknown command :%s", Word);
                }
            }
        }
        else {
            char *String;

            while (*(String = RHS)) {
                cell *Cell = GetNewCell(Row);

                RHS = BreakOffCell(RHS);

                /* TODO: distinguish head, body, and foot. */
                char xxx[] = "!!!";
                if (IsEvalChar(String[0])) {
                    /* TODO: actually deal with command cells */
                    String = xxx;
                }

                BufferString(Cell->Value, ArrayCount(Cell->Value), String);
            }

            Row = GetNewRow(Spreadsheet);;
        }
    }

    Info("We have read the entire file into memory.");

    for (int i = 0; i < Spreadsheet->RowCount; ++i) {
        row *Row = Spreadsheet->Row + i;
        cell *Cell;
        int j;

        for (j = 0; j < Row->CellCount - 1; ++j) {
            Cell = Row->Cell + j;
            Cell->Width = ColWidth[j];

            PrintStringCell(Cell->Value, " ", Cell->Width);
        }

        Cell = Row->Cell + j;
        Cell->Width = ColWidth[j];
        PrintStringCell(Cell->Value, "\n", Cell->Width);
    }

    FreeDocument(Spreadsheet);
    fclose(File);

    return 0;

error:
    if (Spreadsheet) FreeDocument(Spreadsheet);
    if (File) fclose(File);
    return 1;
}
