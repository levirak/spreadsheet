#include <main.h>
#include <sc_strings.h>

#include <sc_mem.h>

#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

/* NOTE: this function does not handle '\r\n' or '\r' line ends */
ssize_t GetLine(char *Buf, mm BufSz, s32 FileHandle) {
    char *End = Buf + BufSz - 1; /* leave space for a '\0' */
    char *Cur;
    char C = '\0';
    s32 BytesRead;

    for (Cur = Buf; Cur < End; ++Cur) {
        /* TODO: buffering to reduce system calls? */
        BytesRead = read(FileHandle, &C, sizeof C);
        if (BytesRead && C != '\n') {
            *Cur = C;
        }
        else {
            break;
        }
    }
    *Cur = '\0';

    /* consume to the End of the line even if we cannot store it */
    while (BytesRead && C != '\n') BytesRead = read(FileHandle, &C, sizeof C);

    if (!BytesRead) {
        return -1;
    }
    else {
        return Cur - Buf;
    }
}

/* NOTE: this function does not validate glyphs */
s32 GlyphCount(char *Str) {
    s32 Count = 0;

    for (char *Cur = Str; *Cur; ++Cur) {
        Count += ((*Cur & '\xc0') != '\x80');
    }

    return Count;
}

mm StringSize(char *Str) {
    char *Cur = Str;
    while (*Cur) ++Cur;
    return (mm)(Cur - Str);
}

char *SkipWord(char *Str) {
    while (*Str && !isspace(*Str)) ++Str;
    return Str;
}

char *SkipSpaces(char *Str) {
    while (*Str && isspace(*Str)) ++Str;
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

s32 CompareString(char *a, char *b) {
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

void PrintCell(document *Doc, cell *Cell, char *Delim, s32 Width, s32 Align) {
    static char Buffer[MAX_COLUMN_WIDTH+1];
    static char *const End = Buffer + ArrayCount(Buffer);

    char *Cur = Buffer;
    char *Value;

    Assert(Cell);

    switch (Cell->ErrorCode) {
    case ERROR_NONE:     Value = GetValue(Doc, Cell); break;
    case ERROR_NOFILE:   Value = "E:nofile";          break;
    case ERROR_NOREF:    Value = "E:noref";           break;
    case ERROR_UNCLOSED: Value = "E:unclosed";        break;
    case ERROR_CYCLE:    Value = "E:cycle";           break;
    case ERROR_RANGE:    Value = "E:range";           break;
    case ERROR_SUB:      Value = "E:sub";             break;
    }

    Assert(Value);

    mm Count = GlyphCount(Value);

    Assert(Width < (s32)ArrayCount(Buffer));

    switch (Align) {
    case ALIGN_LEFT:
        Cur += BufferString(Cur, End - Cur, Value);
        BufferSpaces(Cur, End - Cur, Width - Count);
        break;
    case ALIGN_CENTER: {
        mm TotalSpaces = Width - Count;
        mm FirstSpaces = TotalSpaces / 2;
        mm SecondSpaces = TotalSpaces - FirstSpaces;

        Cur += BufferSpaces(Cur, End - Cur, FirstSpaces);
        Cur += BufferString(Cur, End - Cur, Value);
        BufferSpaces(Cur, End - Cur, SecondSpaces);
    } break;
    case ALIGN_RIGHT:
        Cur += BufferSpaces(Cur, End - Cur, Width - Count);
        BufferString(Cur, End - Cur, Value);
        break;
    default:
        InvalidCodePath;
    }

    printf("%s%s", Buffer, Delim);
}

void PrintNumber(s32 cell, char *delim, s32 width, s32 Align) {
    (void)Align; /* TODO: process fields ourself to add proper alignment */
    printf("%-*d%s", width, cell, delim);
}



mm BufferString(char *Buffer, mm Size, char *String) {
    char *End = Buffer + Size - 1;

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

    *BCur = '\0';

    return SCur - String;
}

mm BufferSpaces(char *Buffer, mm Size, s32 Count) {
    char *End;

    if (Count < 0 || (mm)Count < Size - 1) {
        End = Buffer + Count;
    }
    else {
        End = Buffer + Size - 1;
    }

    char *Cur = Buffer;

    while (Cur < End) {
        *Cur++ = ' ';
    }

    *Cur = '\0';

    return Cur - Buffer;
}

s32 StringToPositiveInt(char *Str) {
    s32 Result = 0;

    while (isdigit(*Str)) {
        Result = 10*Result + (*Str++ - '0');
    }

    if (*Str) Result = -1;

    return Result;
}

s32 StringToInt(char *Str, char **RHS) {
    s32 Result = 0;
    s32 Mod = 1;

    switch (*Str) {
    case '-':
        Mod = -1;
        fallthrough;
    case '+':
        ++Str;
        break;
    }

    while (isdigit(*Str)) {
        Result = 10*Result + (*Str++ - '0');
    }

    *RHS = Str;
    return Mod * Result;
}

r32 StringToReal(char *Str, char **RHS) {
    r32 High = 0;
    r32 Low = 0;
    r32 Fraction = 1;
    r32 Sign = 1;

    switch (*Str) {
    case '-':
        Sign = -1;
        fallthrough;
    case '+':
        ++Str;
        break;
    }

    while (isdigit(*Str)) {
        High = 10*High + (r32)(*Str++ - '0');
        if (*Str == ',') ++Str;
    }

    if (*Str == '.') {
        ++Str;
        while (isdigit(*Str)) {
            Low = 10*Low + (r32)(*Str++ - '0');
            Fraction *= 10;
        }
    }

    *RHS = Str;
    return Sign * (High + Low/Fraction);
}
