#include <main.h>
#include <sc_eval.h>

#include <dbg.h>

#include <sc_strings.h>
#include <sc_mem.h>

#include <ctype.h>
#include <limits.h>

static int StartRow;
static int StartCell;
static int CurrentRow;
static int CurrentCell;
static int EndRow;
static int EndCell;

/* TODO: clean up the return structure to only return from one location */
static inline
int InitRange(char *RangeSpec) {
    /* @TEMP: only 26 columns possible */
    if (!isupper(RangeSpec[0])) return 0;
    if (!isdigit(RangeSpec[1])) return 0;

    char *RHS = BreakAtChar(RangeSpec, '-');

    StartCell = CurrentCell = RangeSpec[0] - 'A';
    StartRow = CurrentRow = StringToPositiveInt(RangeSpec+1);

    if (CurrentRow == -1) return 0;

    if (*RHS == '-') {
        if (*RHS == '-') ++RHS;

        if (*RHS) {
            EndCell = RHS[0] - 'A';
            EndRow = StringToPositiveInt(RHS+1);
            if (EndRow == -1) return 0;
        }
        else {
            EndCell = CurrentCell;
            EndRow = INT_MAX;
        }
    }
    else if (*RHS == '\0') {
        EndRow = CurrentRow;
        EndCell = CurrentCell;
    }
    else {
        Error("Unknown rangespec.");
        return 0;
    }

    return 1;
}

static inline
int GetNextCell(document *Spreadsheet, cell **Cell) {
    int IsValidCell = 0;
    *Cell = NULL;

    if (CurrentRow > EndRow) {
        CurrentRow = StartRow;
        ++CurrentCell;
    }

    if (CurrentCell <= EndCell && CurrentRow < Spreadsheet->RowCount) {
        row *Row = Spreadsheet->Row + CurrentRow;

        IsValidCell = 1;

        if (CurrentCell < Row->CellCount) {
            *Cell = Row->Cell + CurrentCell;
            Info("getting Cell %c%d (%x).", 'A'+CurrentCell, CurrentRow,
                 (*Cell)->Status);
        }

    }

    ++CurrentRow;

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
        Info("Evaluating Cell (%x)", Cell->Status);

        char *FunctionName = Cell->Value + 1;
        char *RHS = BreakAtChar(FunctionName, '(');

        if (CompareString(FunctionName, "sum") == 0) {
            char *Range = RHS;
            RHS = BreakAtChar(Range, ')');
            /* TODO: check RHS for trailing characters */

            if (InitRange(Range)) {
                cell *C;
                int Sum = 0;

                while (GetNextCell(Spreadsheet, &C)) {
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
                            /* TODO: get a real strtol */
                            int i = StringToPositiveInt(C->Value);
                            Info("i = %d", i);

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
