#include <main.h>
#include <sc_eval.h>

#include <dbg.h>

#include <sc_strings.h>
#include <sc_mem.h>

#include <ctype.h>
#include <limits.h>
#include <stdbool.h>

typedef struct range {
    int StartRow;
    int StartCell;
    int CurrentRow;
    int CurrentCell;
    int EndRow;
    int EndCell;
} range;

/* TODO: clean up the return structure to only return from one location */
static inline
bool InitRange(char *RangeSpec, range *Range) {
    /* @TEMP: only 26 columns possible */
    bool IsValidCell = false;

    if (isupper(RangeSpec[0]) && isdigit(RangeSpec[1])) {
        char *RHS = BreakAtChar(RangeSpec, '-');

        Range->StartCell = RangeSpec[0] - 'A';
        Range->StartRow = StringToPositiveInt(RangeSpec+1) - 1;

        Range->CurrentCell = Range->StartCell;
        Range->CurrentRow = Range->StartRow;

        if (Range->CurrentRow > -1) {
            if (*RHS == '-') {
                IsValidCell = true;

                if (*RHS == '-') ++RHS;

                if (*RHS) {
                    Range->EndCell = RHS[0] - 'A';
                    Range->EndRow = StringToPositiveInt(RHS+1) - 1;
                    if (Range->EndRow == -1) return 0;
                }
                else {
                    Range->EndCell = Range->CurrentCell;
                    Range->EndRow = INT_MAX;
                }
            }
            else if (*RHS == '\0') {
                IsValidCell = true;

                Range->EndRow = Range->CurrentRow;
                Range->EndCell = Range->CurrentCell;
            }
            else {
                Error("Unknown rangespec.");
            }
        }
    }

    return IsValidCell;
}

static inline
int GetNextCell(document *Spreadsheet, range *Range, cell **Cell) {
    int IsValidCell = 0;
    *Cell = NULL;

    if (Range->CurrentRow > Range->EndRow) {
        Range->CurrentRow = Range->StartRow;
        ++Range->CurrentCell;
    }

    if (Range->CurrentCell <= Range->EndCell &&
        Range->CurrentRow < Spreadsheet->RowCount) {
        row *Row = Spreadsheet->Row + Range->CurrentRow;

        IsValidCell = 1;

        if (Range->CurrentCell < Row->CellCount) {
            *Cell = Row->Cell + Range->CurrentCell;
        }

    }

    ++Range->CurrentRow;

    return IsValidCell;
}

#define MAX_BUFFER ArrayCount(((cell *)0)->Value)
char *EvaluateCell(document *Spreadsheet, cell *Cell) {
    if (!Cell) return NULL;

    if (Cell->Status & CELL_EVALUATING) {
        Cell->Status |= CELL_CAUSE_ERROR;

        /* TODO: better error message */
        Error("Cycle detected.");
        BufferString(Cell->Value, ArrayCount(Cell->Value), "E:cycle");
    }
    else if (Cell->Status & CELL_FUNCTION) {
        Cell->Status |= CELL_EVALUATING;

        char *FunctionName = Cell->Value + 1;
        char *RHS = BreakAtChar(FunctionName, '(');

        if (CompareString(FunctionName, "sum") == 0) {
            range Range;
            char *RangeSpec = RHS;
            RHS = BreakAtChar(RangeSpec, ')');
            /* TODO: check RHS for trailing characters */

            if (InitRange(RangeSpec, &Range)) {
                cell *C;
                int Sum = 0;

                while (GetNextCell(Spreadsheet, &Range, &C)) {
                    EvaluateCell(Spreadsheet, C);

                    /* pretend that NULL cells evaluate to 0 */
                    if (C) {
                        if (C->Status & CELL_CAUSE_ERROR) {
                            C->Status &= ~CELL_CAUSE_ERROR;
                            Cell->Status |= CELL_ERROR;

                            break;
                        }
                        else if (C->Status & CELL_ERROR) {
                            Cell->Status |= CELL_ERROR;

                            break;
                        }
                        else {
                            int i = StringToInt(C->Value, &RHS);
                            /* TODO: check RHS for trailing characters */
                            Sum += i;
                        }
                    }
                }

                if (Cell->Status & CELL_ERROR) {
                    size_t Size = ArrayCount(Cell->Value);
                    BufferString(Cell->Value, Size, "E:cycle");
                }
                else {
                    snprintf(Cell->Value, ArrayCount(Cell->Value), "%d", Sum);
                }
            }
            else {
                BufferString(Cell->Value, ArrayCount(Cell->Value), "E:range");
            }
        }

        Cell->Status &= ~(CELL_EVALUATING | CELL_FUNCTION);
    }

    return Cell->Value;
}
