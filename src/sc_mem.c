#include <main.h>
#include <sc_mem.h>

#include <stdlib.h>

document *AllocDocument() {
    document *Document;

    Document = malloc(sizeof *Document);
    Document->RowCap = 1;
    Document->RowCount = 0;
    Document->Row = malloc(sizeof *Document->Row * Document->RowCap);

    return Document;
}

void FreeDocument(document *Document) {
    for (int i = 0; i < Document->RowCount; ++i) {
        row *Row = Document->Row + i;

        free(Row->Cell);
    }

    free(Document->Row);
    free(Document);
}

row *GetNewRow(document *Root) {
    row *Row = NULL;

    if (Root) {
        if (Root->RowCount >= Root->RowCap) {
            Root->RowCap *= 2;
            Root->Row = realloc(Root->Row, sizeof *Root->Row * Root->RowCap);
        }

        Row = Root->Row + Root->RowCount++;
        Row->CellCap = 1;
        Row->CellCount = 0;
        Row->Cell = malloc(sizeof *Row->Cell * Row->CellCap);
    }

    return Row;
}

static inline
cell *GetNewCellPreChecked(row *Row) {
    if (Row->CellCount >= Row->CellCap) {
        Row->CellCap *= 2;
        Row->Cell = realloc(Row->Cell, sizeof *Row->Cell * Row->CellCap);
    }

    cell *Cell = Row->Cell + Row->CellCount++;

    return Cell;
}

cell *GetNewCell(row *Root) {
    cell *Cell = NULL;

    if (Root) {
        Cell = GetNewCellPreChecked(Root);
        Cell->Width = DEFAULT_CELL_WIDTH;
    }

    return Cell;
}

void MemSet(void *Destination, size_t Size, char Byte) {
    char *End = (char *)Destination + Size;

    char *Cur = Destination;

    while (Cur < End) {
        *Cur++ = Byte;
    }
}

void MemCopy(void *Destination, size_t Size, void *Source) {
    char *End = (char *)Destination + Size;

    char *DCur = Destination;
    char *SCur = Source;

    while (DCur < End) {
        *DCur++ = *SCur++;
    }
}
