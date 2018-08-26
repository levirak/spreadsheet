#include <main.h>
#include <sc_mem.h>

#include <stdlib.h>

row *AllocRow() {
    row *Row = malloc(sizeof *Row);

    MemZero(Row, sizeof *Row);

    return Row;
}

void FreeRow(row *Row) {
    for (cell *Cell = Row->FirstCell; Cell; Cell = Cell->Next) {
        FreeCell(Cell);
    }

    free(Row);
}

cell *AllocCell(cell *Prototype) {
    cell *Cell = malloc(sizeof *Cell);

    if (Prototype) {
        MemCopy(Cell, sizeof *Cell, Prototype);
        Cell->Next = NULL;
    }
    else {
        MemZero(Cell, sizeof *Cell);
    }

    return Cell;
}

void FreeCell(cell *Cell) {
    free(Cell);
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
