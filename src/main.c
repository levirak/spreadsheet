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
    char buf[1024];
    FILE *file = 0;
    enum row_type RowType = ROW_NONE;

    row *FirstRow = AllocRow();
    row *Row = FirstRow;

    /* @TEMP: this row count
    * Note that for now there can be now more than 26 rows.
    */
    int ColWidth[8];

    /* TODO: true argument parsing */
    Check(argc == 2, "invalid invocation");

    Check(file = fopen(argv[1], "r"), "failed to open file");

    /* initialize ColWidth */
    for (size_t i = 0; i < ArrayCount(ColWidth); ++i) ColWidth[i] = 8;

    while (GetLine(buf, ArrayCount(buf), file) > 0) {
        int Column = 0;
        char *rhs = buf;

        if (IsCommentChar(rhs[0])) {
            if (IsCommandChar(rhs[1])) {
                char *Word = rhs + 2;
                rhs = BreakOffWord(Word);

                /* process commands */

                if (CompareString(Word, "width") == 0) {
                    while (*(Word = rhs)) {
                        rhs = BreakOffWord(rhs);

                        /* TODO: check for errors from strtol */
                        ColWidth[Column++] = strtol(Word, NULL, 0);
                    }
                }
                else if (CompareString(Word, "print") == 0) {
                    Word = rhs;
                    rhs = BreakOffWord(rhs);

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
                    Word = rhs;
                    rhs = BreakOffWord(rhs);

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
            cell *Cell = Row->FirstCell = AllocCell(NULL);

            while (*(String = rhs)) {
                rhs = BreakOffCell(rhs);

                /* TODO: distinguish head, body, and foot. */
                char xxx[] = "!!!";
                if (IsEvalChar(String[0])) {
                    /* TODO: actually deal with command cells */
                    String = xxx;
                }

                BufferString(Cell->Value, ArrayCount(Cell->Value), String);

                Cell->Next = AllocCell(NULL);
                Cell = Cell->Next;
            }

            Row->Next = AllocRow();
            Row = Row->Next;
        }
    }

    Info("We have read the entire file into memory.");

    for (Row = FirstRow; Row; Row = Row->Next) {
        int Column = 0;
        for (cell *Cell = Row->FirstCell; Cell; Cell = Cell->Next) {
            int Width = ColWidth[Column++];
            PrintStringCell(Cell->Value, Cell->Next? " ": "\n", Width);
        }
    }

    fclose(file);

    return 0;

error:
    if (file) fclose(file);
    return 1;
}
