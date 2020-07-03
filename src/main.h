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
#   define CheckEq(E, ...) E
#   define CheckNe(E, ...) E
#   define CheckGt(E, ...) E
#   define CheckLt(E, ...) E
#   define CheckGe(E, ...) E
#   define CheckLe(E, ...) E
#   define InvalidCodePath __builtin_trap()
#   define static_assert(...) _Static_assert(__VA_ARGS__)
#else
#   include <assert.h>
#   define Assert(E) assert(E)
#   define CheckEq(E, V) assert((E) == (V))
#   define CheckNe(E, V) assert((E) != (V))
#   define CheckGt(E, V) assert((E) > (V))
#   define CheckLt(E, V) assert((E) < (V))
#   define CheckGe(E, V) assert((E) >= (V))
#   define CheckLe(E, V) assert((E) <= (V))
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

typedef intptr_t  sptr;
typedef uintptr_t ptr;

#include <stddef.h>
typedef ptrdiff_t dptr;
typedef size_t    mm;
typedef ssize_t   smm;

#include <stdbool.h>

typedef float  r32;
typedef double r64;

typedef int fd;

/* put the cannary into its cage */
static_assert(sizeof (ptr) == sizeof (sptr));
static_assert(sizeof (dptr) == sizeof (sptr));
static_assert(sizeof (dptr) == sizeof (ptr));
static_assert(sizeof (mm) == sizeof (ptr));
static_assert(sizeof (mm) == sizeof (smm));
static_assert(sizeof (smm) == sizeof (dptr));

#define fallthrough __attribute__((fallthrough))
/*#define macro static __attribute__((always_inline))*/
#define inline inline __attribute((gnu_inline))
#define noreturn _Noreturn
#define alignof _Alignof

#endif
