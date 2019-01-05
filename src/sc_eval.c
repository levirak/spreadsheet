#include <main.h>
#include <sc_eval.h>

#include <dbg.h>

#include <sc_strings.h>
#include <sc_mem.h>

#include <ctype.h>
#include <limits.h>
#include <stdbool.h>

#include <fcntl.h>
#include <unistd.h>

typedef struct {
    int StartRow;
    int StartCell;
    int CurrentRow;
    int CurrentCell;
    int EndRow;
    int EndCell;
} range;

static inline
bool IsReference(char *RefSpec) {
    /* @TEMP: only 26 columns possible */
    bool Result = isupper(RefSpec[0]) && isdigit(RefSpec[1]);
    /* TODO: Continue on to show that this is a well formed refenerce */
    return Result;
}

static inline
cell *GetCell(document *Sheet, char *RefSpec) {
    cell *Cell = NULL;

    int RowIdx = StringToPositiveInt(RefSpec+1) - 1;

    if (RowIdx != -1 && RowIdx < Sheet->RowCount) {
        row *Row = Sheet->Row + RowIdx;
        int CellIdx = *RefSpec - 'A';

        if (CellIdx < Row->CellCount) {
            Cell = Row->Cell + CellIdx;
        }
    }

    return Cell;
}


static inline
bool InitRange(char *RangeSpec, range *Range) {
    /* @TEMP: only 26 columns possible */
    bool IsValidCell = false;
    char *RHS;

    if (IsReference(RangeSpec)) {
        Range->StartCell = RangeSpec[0] - 'A';
        Range->StartRow = StringToInt(RangeSpec+1, &RHS) - 1;

        Range->CurrentCell = Range->StartCell;
        Range->CurrentRow = Range->StartRow;

        if (*RHS == ':') {
            IsValidCell = true;

            ++RHS;
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

    return IsValidCell;
}

static inline
int GetNextCell(document *Sheet, range *Range, cell **Cell) {
    int IsValidCell = 0;
    *Cell = NULL;

    if (Range->CurrentRow > Range->EndRow) {
        Range->CurrentRow = Range->StartRow;
        ++Range->CurrentCell;
    }

    if (Range->CurrentCell <= Range->EndCell &&
        Range->CurrentRow < Sheet->RowCount) {
        row *Row = Sheet->Row + Range->CurrentRow;

        IsValidCell = 1;

        if (Range->CurrentCell < Row->CellCount) {
            *Cell = Row->Cell + Range->CurrentCell;
        }

    }

    ++Range->CurrentRow;

    return IsValidCell;
}

#define MAX_BUFFER ArrayCount(((cell *)0)->Value)
char *EvaluateCell(document *Sheet, cell *Cell) {
    if (!Cell) return NULL;

    if (Cell->Status & CELL_EVALUATING) {
        Cell->Status |= CELL_CLOSE_CYCLE;
    }
    else if (Cell->Status & CELL_FUNCTION) {
        Cell->Status |= CELL_EVALUATING;

        char *FunctionName = Cell->Value + 1;
        char *RHS = BreakAtChar(FunctionName, '(');

        /* TODO: make a true and proper expression language.
         *
         * By this, I mean that roughly all of the following should work at
         * some point in the future:
         *      =A3
         *      =A3/A2
         *      =-A3
         *      =Sum(A3:A7)
         *      =Sum(A3:A7)/Count(A3:A7)
         *      =Sum(A3:A7)/N(A3:A7)
         *      =Average(A3:A7)
         *      =Avg(A3:A7)
         *      =A3 + 2
         *      =20% * A3
         *      =1/4
         *      =5
         *      =@r1
         *      ={./other.tsv:A1}
         *      ={june.tsv:A1}
         *      ={/path/to/sheet.tsv:A1}
         *
         * The spec would basically be that---
         *      FN       := [a-zA-Z]+ '(' ARGUMENTS ')'
         *      ABS_REF  := [A-Z]+[0-9]+
         *      REL_REF  := '@'([ud][0-9]+)?([lr][0-9]+)?
         *      EX_REF   := '{' PATH '.' IN_RANGE '}'
         *      IN_REF   := ABS_REF | REL_REF
         *      REF      := IN_REF | EX_REF
         *      NUM      := [+-]?[0-9]+
         *               |  [+-]?[0-9]*'.'[0-9]+
         *               |  [+-]?[0-9]+'%'
         *      EXPR     := FN | REF | NUM
         *               |  EXPR [+-*^/] EXPR
         *               |  '(' EXPR ')'
         *      IN_RANGE := IN_REF':'IN_REF
         *               |  IN_REF':'[udlr]?
         *      RANGE    := REF | IN_RANGE
         *      LIST     := RANGE (';' RANGE)*
         *      COMP     := EXPR ('<' | '<=' | '=' | '=>' | '>' | '<>') EXPR
         *               |  COMP ('&&'|'||') COMP
         *               |  '(' COMP ')'
         * We would then expect to find an EXPR (expression) in any cell
         * starting with '='
         *
         * edit: yeah, "basically" like that
         */

        if (FunctionName[0] == '{') {
            size_t Size = ArrayCount(Cell->Value);

            char *RHS = BreakAtLastChar(FunctionName, '}');
            if (RHS) {
                ++FunctionName;
                RHS = BreakAtLastChar(FunctionName, ':');

                if (RHS && IsReference(RHS)) {
                    /* TODO: cache this document (?), so that multiple
                     * references don't pull it in freash every time */
                    document *Sub = ReadSheetAt(Sheet->DirFD, FunctionName);
                    if (Sub) {
                        char *Value = EvaluateCell(Sub, GetCell(Sub, RHS));

                        BufferString(Cell->Value, Size, Value);

                        FreeDocument(Sub);
                    }
                    else {
                        Cell->Status |= CELL_ERROR;
                        BufferString(Cell->Value, Size, "E:nofile");
                    }
                }
                else {
                    Cell->Status |= CELL_ERROR;
                    BufferString(Cell->Value, Size, "E:noref");
                }
            }
            else {
                Cell->Status |= CELL_ERROR;
                BufferString(Cell->Value, Size, "E:unclosed");
            }
        }
        else if (IsReference(FunctionName)) {
            cell *C = GetCell(Sheet, FunctionName);

            if (C) {
                char *Value = EvaluateCell(Sheet, C);

                if (C->Status & CELL_CLOSE_CYCLE) {
                    C->Status &= ~CELL_CLOSE_CYCLE;
                    Cell->Status |= CELL_ERROR;
                    Value = "E:cycle";
                }

                BufferString(Cell->Value, ArrayCount(Cell->Value), Value);
            }
        }
        else if (CompareString(FunctionName, "sum") == 0) {
            range Range;
            char *RangeSpec = RHS;
            RHS = BreakAtChar(RangeSpec, ')');
            /* TODO: check RHS for trailing characters */

            if (InitRange(RangeSpec, &Range)) {
                char *ErrorString = NULL;
                float Sum = 0;
                cell *C;

                while (GetNextCell(Sheet, &Range, &C)) {
                    /* pretend that NULL cells evaluate to 0 */
                    if (C) {
                        EvaluateCell(Sheet, C);

                        if (C->Status & CELL_CLOSE_CYCLE) {
                            C->Status &= ~CELL_CLOSE_CYCLE;
                            Cell->Status |= CELL_ERROR;
                            ErrorString = "E:cycle";

                            break;
                        }
                        else if (C->Status & CELL_ERROR) {
                            Cell->Status |= CELL_ERROR;
                            ErrorString = "E:Sub";

                            break;
                        }
                        else {
                            float Value = StringToReal(C->Value, &RHS);
                            /* TODO: check RHS for trailing characters */
                            Sum += Value;
                        }
                    }
                }

                if (ErrorString) {
                    size_t Size = ArrayCount(Cell->Value);
                    BufferString(Cell->Value, Size, ErrorString);
                }
                else {
                    /* TODO: roll our own snprintf */
                    snprintf(Cell->Value, ArrayCount(Cell->Value), "%f", Sum);
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
