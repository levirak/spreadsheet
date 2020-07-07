#ifndef sc_eval
#define sc_eval

#include "mem.h"

cell_value Compile(document *Document, char *Raw, enum cell_error_code *Error);
cell_value EvaluateCell(document *Spreadsheet, cell *Cell);

#endif
