#include <main.h>
#include <dbg.h> /* TODO: is Zed Shaw's dbg.h good? */

#include <sc_strings.h>

#include <stdio.h>
/*#include <ctype.h>*/
#include <stdlib.h>

int main(int argc, char **argv) {
    char buf[1024];
    FILE *file = 0;

    /* @TEMP: this row count
    * Note that for now there can be now more than 26 rows.
    */
    int rwidth[8];

    /* TODO: true argument parsing */
    check(argc == 2, "invalid invocation");

    check(file = fopen(argv[1], "r"), "failed to open file");

    for (size_t i = 0; i < ArrayCount(rwidth); ++i) rwidth[i] = 8;

    while (GetLine(buf, ArrayCount(buf), file) > 0) {
        int rcount = 0;
        char *rhs = buf;

        if (IsCommentChar(rhs[0])) {
            if (IsCommandChar(rhs[1])) {
                char *word = rhs + 2;
                rhs = BreakOffWord(word);

                if (CompareString(word, "width") == 0) {
                    while (*(word = rhs)) {
                        rhs = BreakOffWord(rhs);

                        /* TODO: check for errors from strtol */
                        rwidth[rcount++] = strtol(word, NULL, 0);
                    }
                }
                else if (CompareString(word, "print") == 0) {
                    word = rhs;
                    rhs = BreakOffWord(rhs);

                    if (CompareString(word, "top_axis") == 0) {
                        for (size_t i = 0; i < ArrayCount(rwidth); ++i) {
                            char name[2] = {0};
                            name[0] = 'A' + i;
                            PrintStringCell(name, DelimFor(i), rwidth[i]);
                        }
                    }
                    else if (CompareString(word, "width") == 0) {
                        for (size_t i = 0; i < ArrayCount(rwidth); ++i) {
                            PrintNumCell(rwidth[i], DelimFor(i), rwidth[i]);
                        }
                    }
                }
            }
        }
        else {
            char *cell;
            while (*(cell = rhs)) {
                rhs = BreakOffCell(rhs);

                /* TODO: actually deal with command cells */
                char xxx[] = "!!!";
                if (IsCommandChar(cell[0])) {
                    cell = xxx;
                }

                PrintStringCell(cell, *rhs? " ": "\n", rwidth[rcount++]);
            }
        }
    }

    fclose(file);

    return 0;

error:
    if (file) fclose(file);
    return 1;
}
