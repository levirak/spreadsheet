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

cell_value AdoptValue(document *Document, cell_value Value) {
    if (Value.Type == CELL_TYPE_STRING) {
        Value.AsString = PushString(Document, Value.AsString);
    }

    return Value;
}

typedef struct expr {
    enum {
        EXPR_NONE = 0,
        EXPR_FUNC,
        EXPR_INT_REF,
        EXPR_EXT_REF,
    } Type;
    union {
        struct {
            char *Reference;
            char *FileName;
        } AsExtRef;
        char *AsIntRef;
        struct {
            char *Name;
            char *Param1;
        } AsFunc;
    };
} expr;

cell_value Compile(document *Doc, char *Raw, enum cell_error_code *Error) {
    Assert(Doc);
    Assert(Raw);
    Assert(Error);

    /* TODO: make a true and proper expression language.
     *
     * By this, I mean that roughly all of the following should work at
     * some point in the future:
     *      A3
     *      A3/A2
     *      -A3
     *      Sum(A3:A7)
     *      Sum(A3:A7)/Count(A3:A7)
     *      Sum(A3:A7)/N(A3:A7)
     *      Average(A3:A7)
     *      Avg(A3:A7)
     *      A3 + 2
     *      20% * A3
     *      1/4
     *      5
     *      @r1
     *      {./other.tsv:A1}
     *      {june.tsv:A1}
     *      {/path/to/sheet.tsv:A1}
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
    *Error = 0;
    expr *Expr = 0;

    char *FunctionName = Raw;
    char *RHS = BreakAtChar(FunctionName, '(');

    if (FunctionName[0] == '{') {
        char *RHS = BreakAtLastChar(FunctionName, '}');

        if (!RHS) {
            *Error = ERROR_UNCLOSED;
        }
        else if (*RHS) {
            *Error = ERROR_TRAIL;
        }
        else {
            RHS = BreakAtLastChar(++FunctionName, ':');

            if (!*FunctionName) {
                *Error = ERROR_NOFILE;
            }
            else if (!RHS || !IsReference(RHS)) {
                *Error = ERROR_NOREF;
            }
            else {
                Expr = PushSize(Doc, sizeof *Expr, alignof *Expr);
                *Expr = (expr){
                    .Type = EXPR_EXT_REF,
                    .AsExtRef = {
                        .FileName = PushString(Doc, FunctionName),
                        .Reference = PushString(Doc, RHS),
                    },
                };
            }
        }
    }
    else if (IsReference(FunctionName)) {
        Expr = PushSize(Doc, sizeof *Expr, alignof *Expr);
        *Expr = (expr){
            .Type = EXPR_INT_REF,
            .AsIntRef = PushString(Doc, FunctionName),
        };
    }
    else {
        Assert(RHS);

        char *RangeSpec = RHS;
        RHS = BreakAtLastChar(RangeSpec, ')');

        if (!RHS) {
            *Error = ERROR_UNCLOSED;
        }
        else if (*RHS) {
            *Error = ERROR_TRAIL;
        }
        else {
            Expr = PushSize(Doc, sizeof *Expr, alignof *Expr);
            *Expr = (expr){
                .Type = EXPR_FUNC,
                .AsFunc = {
                    .Name = PushString(Doc, FunctionName),
                    .Param1 = PushString(Doc, RangeSpec),
                },
            };
        }
    }

    return (cell_value){
        .Type = CELL_TYPE_EXPR,
        .AsExpr = Expr,
    };
}

cell_value EvaluateCell(document *Doc, cell *Cell) {
    Assert(Cell);

    if (Cell->Value.Type == CELL_TYPE_EXPR) {
        if (Cell->Status & CELL_EVALUATING) {
            Cell->Status |= CELL_CLOSE_CYCLE;
        }
        else if (!Cell->ErrorCode) {
            Cell->Status |= CELL_EVALUATING;

            struct expr *Expr = Cell->Value.AsExpr;

            if (Expr) switch (Expr->Type) {
            case EXPR_NONE: { /* noop */ } break;

            case EXPR_INT_REF: {
                cell *SubCell = GetRefCell(Doc, Expr->AsIntRef);

                if (SubCell) {
                    Cell->Value = EvaluateCell(Doc, SubCell);

                    if (SubCell->Status & CELL_CLOSE_CYCLE) {
                        SubCell->Status &= ~CELL_CLOSE_CYCLE;
                        Cell->ErrorCode = ERROR_CYCLE;
                    }
                }
            } break;

            case EXPR_EXT_REF: {
                char *FileName = Expr->AsExtRef.FileName;

                /* TODO: cache this document (?), so that multiple
                 * references don't pull it in freash every time */
                document *Sub = ReadDocumentRelativeTo(Doc, FileName);

                if (!Sub) {
                    Cell->ErrorCode = ERROR_NOFILE;
                }
                else {
                    char *CellRef = Expr->AsExtRef.Reference;

                    cell *SubCell = GetRefCell(Sub, CellRef);
                    cell_value Value = EvaluateCell(Sub, SubCell);

                    Cell->Value = AdoptValue(Doc, Value);

                    FreeDocument(Sub);
                }
            } break;

            case EXPR_FUNC: {
                if (CompareString(Expr->AsFunc.Name, "sum") == 0) {
                    range Range;

                    if (!InitRange(Expr->AsFunc.Param1, &Range)) {
                        Cell->ErrorCode = ERROR_RANGE;
                    }
                    else {
                        r32 Sum = 0;
                        cell *C;

                        while (GetNextCell(Doc, &Range, &C)) {
                            /* pretend that NULL cells evaluate to 0 */
                            if (C) {
                                EvaluateCell(Doc, C);

                                if (C->Status & CELL_CLOSE_CYCLE) {
                                    C->Status &= ~CELL_CLOSE_CYCLE;
                                    Cell->ErrorCode = ERROR_CYCLE;

                                    break;
                                }
                                else if (C->ErrorCode) {
                                    Sum += 0.0f;
                                }
                                else {
                                    r32 Value = 0;

                                    switch (C->Value.Type) {
                                    case CELL_TYPE_REAL:
                                        Value = C->Value.AsReal;
                                        break;
                                    case CELL_TYPE_INT:
                                        Value = C->Value.AsInt;
                                        break;
                                    default: break;
                                    }

                                    Sum += Value;
                                }
                            }
                        }

                        if (!Cell->ErrorCode) {
                            Cell->Value = (cell_value){
                                .Type = CELL_TYPE_REAL,
                                .AsReal = Sum,
                            };
                        }
                    }
                }
                else {
                    Cell->ErrorCode = ERROR_BAD_FUNC;
                }
            } break;

            default: InvalidCodePath;
            }

            Cell->Status &= ~CELL_EVALUATING;
        }
    }

    return Cell->Value;
}
