#include <main.h>
#include <sc_strings.h>

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

/* NOTE: this function does not handle '\r\n' or '\r' line ends */
ssize_t GetLine(char *Buffer, size_t BufferSize, FILE *File) {
    char *End = Buffer + BufferSize - 1; /* leave space for a '\0' */
    char *Cur;
    char C = '\0';

    for (Cur = Buffer; Cur < End; ++Cur) {
        C = fgetc(File);
        if (C == EOF || C == '\n') {
            break;
        }
        else {
            *Cur = C;
        }
    }
    *Cur = '\0';

    /* consume to the End of the line even if we cannot store it */
    while (C != EOF && C != '\n') C = fgetc(File);

    if (C == EOF) {
        return -1;
    }
    else {
        return Cur - Buffer;
    }
}

/* NOTE: this function does not validate glyphs */
int GlyphCount(char *Str) {
    int count = 0;

    for (char *Cur = Str; *Cur; ++Cur) {
        count += ((*Cur & '\xc0') != '\x80');
    }

    return count;
}

size_t StringSize(char *Str) {
    char *Cur = Str;
    while (*Cur) ++Cur;
    return (size_t)Cur - (size_t)Str;
}

char *SkipWord(char *Str) {
    while (*Str && !isspace(*Str)) ++Str;
    return Str;
}

char *SkipSpaces(char *Str) {
    while (isspace(*Str)) ++Str;
    return Str;
}

char *BreakOffWord(char *Str) {
    Str = SkipWord(Str);
    if (*Str) *Str++ = '\0';
    Str = SkipSpaces(Str);
    return Str;
}

char *BreakAtChar(char *Str, char Delim) {
    while (*Str && *Str != Delim) ++Str;
    if (*Str) *Str++ = '\0';
    return Str;
}

char *BreakAtLastChar(char *Str, char Delim) {
    char *Hold;

    for (Hold = NULL; *Str; ++Str) {
        if (*Str == Delim) {
            Hold = Str;
        }
    }

    if (Hold) *Hold++ = '\0';

    return Hold;
}

char *FindChar(char *Str, char Delim) {
    while (*Str && *Str != Delim) ++Str;
    return Str;
}

int CompareString(char *a, char *b) {
    while (*a && *a == *b) {
        ++a;
        ++b;
    }
    return *a - *b;
}

char *Strip(char *Str) {
    char *hold, *Cur;

    Str = hold = Cur = SkipSpaces(Str);
    while (*Cur && !IsCommentChar(*Cur)) {
        hold = Cur = SkipWord(Cur);
        Cur = SkipSpaces(Cur);
    }
    *hold = '\0';

    return Str;
}

void PrintStringCell(char *Cell, char *Delim, int Width) {
    char *Cur;
    char Deleted = '\0';
    int Count = 0;
    for (Cur = Cell; *Cur; ++Cur) {
        Count += ((*Cur & '\xc0') != '\x80');

        /* TODO: glyphs with double width */
        if (Count > Width) {
            Deleted = *Cur;
            *Cur = '\0';
            break;
        }
    }

    /* TODO: process fields ourself to add proper alignment */
    printf("%-*s%s", Width, Strip(Cell), Delim);

    *Cur = Deleted;
}

void PrintNumCell(int cell, char *delim, int width) {
    /* TODO: process fields ourself to add proper alignment */
    printf("%-*d%s", width, cell, delim);
}



size_t BufferString(char *Buffer, size_t Size, char *String) {
    char *End = Buffer + Size;

    char *BCur = Buffer;
    char *SCur = String;

    while (BCur < End) {
        *BCur++ = *SCur;

        if (*SCur) {
            ++SCur;
        }
        else {
            break;
        }
    }

    return SCur - String;
}

int StringToPositiveInt(char *Str) {
    int Result = 0;

    while (isdigit(*Str)) {
        Result = 10*Result + (*Str++ - '0');
    }

    if (*Str) Result = -1;

    return Result;
}

int StringToInt(char *Str, char **RHS) {
    int Result = 0;
    int Mod = 1;

    switch (*Str) {
    case '-':
        Mod = -1;
    fallthrough case '+':
        ++Str;
        break;
    }

    while (isdigit(*Str)) {
        Result = 10*Result + (*Str++ - '0');
    }

    *RHS = Str;
    return Mod * Result;
}

float StringToReal(char *Str, char **RHS) {
    float High = 0;
    float Low = 0;
    float Fraction = 1;
    float Sign = 1;

    switch (*Str) {
    case '-':
        Sign = -1;
    fallthrough case '+':
        ++Str;
        break;
    }

    while (isdigit(*Str)) {
        High = 10*High + (float)(*Str++ - '0');
    }

    if (*Str == '.') {
        ++Str;
        while (isdigit(*Str)) {
            Low = 10*Low + (float)(*Str++ - '0');
            Fraction *= 10;
        }
    }

    *RHS = Str;
    return Sign * (High + Low/Fraction);
}
