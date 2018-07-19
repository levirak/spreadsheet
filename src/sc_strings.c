#include <main.h>
#include <sc_strings.h>

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

ssize_t GetLine(char *buf, size_t bufsz, FILE *file) {
    char *end = buf + bufsz - 1; /* leave space for a '\0' */
    char *cur = buf;
    char c;

    while (cur < end) {
        if ((c = fgetc(file)) == EOF) return -1;
        if ((*cur++ = c) == '\n') break;

    }

    /* consume to the end of the line even if we cannot store it */
    while (c != EOF && c != '\n') c = fgetc(file);

    *cur = '\0';
    return cur - buf;
}

/* NOTE: this function does not validate glyphs */
int GlyphCount(char *str) {
    int count = 0;

    for (char *cur = str; *cur; ++cur) {
        count += ((*cur & '\xc0') != '\x80');
    }

    return count;
}

char *SkipWord(char *str) {
    while (*str && !isspace(*str)) ++str;
    return str;
}

char *SkipSpaces(char *str) {
    while (isspace(*str)) ++str;
    return str;
}

char *BreakOffWord(char *str) {
    str = SkipWord(str);
    if (*str) *str++ = '\0';
    str = SkipSpaces(str);
    return str;
}

char *BreakOffCell(char *str) {
    while (*str && !IsDelimChar(*str)) ++str;
    if (*str) *str++ = '\0';
    return str;
}

int CompareString(char *a, char *b) {
    while (*a && *a == *b) {
        ++a;
        ++b;
    }
    return *a - *b;
}

char *Strip(char *str) {
    char *hold, *cur;

    str = hold = cur = SkipSpaces(str);
    while (*cur && !IsCommentChar(*cur)) {
        hold = cur = SkipWord(cur);
        cur = SkipSpaces(cur);
    }
    *hold = '\0';

    return str;
}

void PrintStringCell(char *cell, char *delim, int width) {
    /* TODO: scan forward for the (width+1)th glyph and set that */
    /*
    if (width < GlyphCount(cell))
        cell[width] = '\0';
     */
    int count = 0;
    for (char *cur = cell; *cur; ++cur) {
        count += ((*cur & '\xc0') != '\x80');
        if (count > width) {
            *cur = '\0';
            break;
        }
    }

    /* TODO: process fields ourself to add proper alignment */
    printf("%-*s%s", width, Strip(cell), delim);
}

void PrintNumCell(int cell, char *delim, int width) {
    /* TODO: process fields ourself to add proper alignment */
    printf("%-*d%s", width, cell, delim);
}
