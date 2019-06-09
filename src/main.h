#ifndef main_h
#define main_h

/* program universal constants */

#define DELIM_CHAR   '\t'
#define COMMENT_CHAR '#'
#define COMMAND_CHAR ':'
#define EVAL_CHAR    '='

#define IsDelimChar(C) ((C) == DELIM_CHAR)
#define IsCommentChar(C) ((C) == COMMENT_CHAR)
#define IsCommandChar(C) ((C) == COMMAND_CHAR)
#define IsEvalChar(C) ((C) == EVAL_CHAR)

#define INNER_FS " "
#define OUTER_FS "\n"
#define MARGIN_FS "  "

/* generic things */

#ifdef NDEBUG
#   define Assert(...)
#   define CheckPos(E, ...) E
#   define CheckNeg(E, ...) E
#   define InvalidCodePath
#else
#   include <assert.h>
#   define Assert(E) assert(E)
#   define CheckPos(E, V) assert(V == (E))
#   define CheckNeg(E, V) assert(V != (E))
#   include <stdlib.h>
#   define InvalidCodePath abort()
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>

/* TODO: get rid of these? */
#define PrintError(M, ...)   fprintf(stderr, "[ERROR] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, errno? strerror(errno): "None", ##__VA_ARGS__)
#define PrintWarning(M, ...) fprintf(stderr, "[WARN] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, errno? strerror(errno): "None", ##__VA_ARGS__)
#define PrintInfo(M, ...)    fprintf(stderr, "[INFO] (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define ArrayCount(A) (sizeof A / sizeof *A)

#include <stdint.h>
typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#include <stdbool.h>

typedef float  r32;
typedef double r64;

typedef int fd;
typedef int pid;

#define fallthrough __attribute__((fallthrough))
/*#define macro static __attribute__((always_inline))*/
#define inline inline __attribute((gnu_inline))

#endif
