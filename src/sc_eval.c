#include <main.h>
#include <sc_eval.h>

#include <sc_strings.h>
#include <sc_mem.h>

#include <ctype.h>
#include <limits.h>
#include <stdbool.h>

#include <fcntl.h>
#include <unistd.h>

typedef struct {
    s32 StartColumn;
    s32 StartRow;
    s32 CurrentColumn;
    s32 CurrentRow;
    s32 EndColumn;
    s32 EndRow;
} range;

static inline
bool IsReference(char *RefSpec) {
    /* @TEMP: only 26 columns possible */
    bool Result = isupper(RefSpec[0]) && isdigit(RefSpec[1]);
    /* TODO: Continue on to show that this is a well formed refenerce */
    return Result;
}

static inline
cell *GetRefCell(document *Doc, char *RefSpec) {
    cell *Cell = NULL;

    /* TODO: more columns */
    s32 ColumnIndex = *RefSpec - 'A';
    s32 RowIndex = StringToPositiveInt(RefSpec+1) - 1;

    if (ColumnIndex < Doc->ColumnCount) {
        column *Column = Doc->Column + ColumnIndex;
        if (RowIndex != -1 && RowIndex < Column->CellCount) {
            Cell = Column->Cell + RowIndex;
        }
    }

    return Cell;
}


static inline
s32 InitRange(char *RangeSpec, range *Range) {
    /* @TEMP: only 26 columns possible */
    s32 IsValidCell = 0;
    char *RHS;

    if (IsReference(RangeSpec)) {
        Range->StartColumn = RangeSpec[0] - 'A';
        Range->StartRow = StringToInt(RangeSpec+1, &RHS) - 1;

        Range->CurrentColumn = Range->StartColumn;
        Range->CurrentRow = Range->StartRow;

        if (*RHS == ':') {
            IsValidCell = 1;

            ++RHS;
            if (*RHS) {
                Range->EndColumn = RHS[0] - 'A';
                Range->EndRow = StringToPositiveInt(RHS+1) - 1;
                if (Range->EndRow == -1) return 0;
            }
            else {
                Range->EndColumn = Range->CurrentColumn;
                Range->EndRow = INT_MAX;
            }
        }
        else if (*RHS == '\0') {
            IsValidCell = 1;

            Range->EndRow = Range->CurrentRow;
            Range->EndColumn = Range->CurrentColumn;
        }
        else {
            PrintError("Unknown rangespec.");
        }
    }

    return IsValidCell;
}

static inline
s32 GetNextCell(document *Doc, range *Range, cell **Cell) {
    s32 CellExists = 0;
    *Cell = NULL;

    /* TODO: simplify logic or redesign ranges */

    if (Range->CurrentColumn < Doc->ColumnCount
    &&  Range->CurrentColumn <= Range->EndColumn) {
        column *Column = Doc->Column + Range->CurrentColumn;

        /* detect overflow */
        if (Range->CurrentRow >= Column->CellCount
        ||  Range->CurrentRow > Range->EndRow) {
            Range->CurrentRow = Range->StartRow;
            ++Range->CurrentColumn;

            if (Range->CurrentColumn < Doc->ColumnCount
            &&  Range->CurrentColumn <= Range->EndColumn) {
                ++Column;
            }
            else {
                return 0;
            }
        }

        *Cell = Column->Cell + Range->CurrentRow++;
        CellExists = 1;
    }

    return CellExists;
}

#define MAX_BUFFER ArrayCount(((cell *)0)->Value)
char *EvaluateCell(document *Document, cell *Cell) {
    if (!Cell) return NULL;

    if (Cell->Status & CELL_EVALUATING) {
        Cell->Status |= CELL_CLOSE_CYCLE;
    }
    else if (Cell->Status & CELL_FUNCTION) {
        Cell->Status |= CELL_EVALUATING;

        Assert(GetValue(Document, Cell)[0] == EVAL_CHAR);

        char *FunctionName = GetValue(Document, Cell) + 1;
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
            char *RHS = BreakAtLastChar(FunctionName, '}');
            if (RHS) {
                ++FunctionName;
                RHS = BreakAtLastChar(FunctionName, ':');

                if (RHS && IsReference(RHS)) {
                    /* TODO: cache this document (?), so that multiple
                     * references don't pull it in freash every time */
                    document *Sub = ReadDocumentRelativeTo(Document,
                                                           FunctionName);
                    if (Sub) {
                        char *Value = EvaluateCell(Sub, GetRefCell(Sub, RHS));
                        Cell->Offset = PushString(Document, Value);

                        FreeDocument(Sub);
                    }
                    else {
                        Cell->ErrorCode = ERROR_NOFILE;
                    }
                }
                else {
                    Cell->ErrorCode = ERROR_NOREF;
                }
            }
            else {
                Cell->ErrorCode = ERROR_UNCLOSED;
            }
        }
        else if (IsReference(FunctionName)) {
            cell *C = GetRefCell(Document, FunctionName);

            if (C) {
                char *Value = EvaluateCell(Document, C);

                if (C->Status & CELL_CLOSE_CYCLE) {
                    C->Status &= ~CELL_CLOSE_CYCLE;
                    Cell->ErrorCode = ERROR_CYCLE;
                }
                else {
                    Cell->Offset = PushString(Document, Value);
                }
            }
        }
        else if (CompareString(FunctionName, "sum") == 0) {
            range Range;
            char *RangeSpec = RHS;
            RHS = BreakAtChar(RangeSpec, ')');
            /* TODO: check RHS for trailing characters */

            if (InitRange(RangeSpec, &Range)) {
                r32 Sum = 0;
                cell *C;

                while (GetNextCell(Document, &Range, &C)) {
                    /* pretend that NULL cells evaluate to 0 */
                    if (C) {
                        EvaluateCell(Document, C);

                        if (C->Status & CELL_CLOSE_CYCLE) {
                            C->Status &= ~CELL_CLOSE_CYCLE;
                            Cell->ErrorCode = ERROR_CYCLE;

                            break;
                        }
                        else if (C->ErrorCode) {
                            Cell->ErrorCode = ERROR_SUB;

                            break;
                        }
                        else {
                            char *Str = GetValue(Document, C);
                            r32 Real = StringToReal(SkipSpaces(Str), &RHS);
                            /* TODO: check RHS for trailing characters */
                            Sum += Real;
                        }
                    }
                }

                if (!Cell->ErrorCode) {
                    char Buffer[128]; /* TMP! */
                    /* TODO: roll our own snprintf */
                    snprintf(Buffer, ArrayCount(Buffer), "%.2f", Sum);
                    Cell->Offset = PushString(Document, Buffer);
                }
            }
            else {
                Cell->ErrorCode = ERROR_RANGE;
            }
        }

        Cell->Status &= ~(CELL_EVALUATING | CELL_FUNCTION);
    }

    return GetValue(Document, Cell);
}
