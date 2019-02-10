#ifndef __main_h__
#define __main_h__

#ifdef NDEBUG
#   define Assert(E)
#   define InvalidCodePath
#else
#   include <assert.h>
#   define Assert(E) assert(E)
#   include <stdlib.h>
#   define InvalidCodePath abort()
#endif

#define ArrayCount(A) (sizeof (A) / sizeof *(A))

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

#define fallthrough __attribute__((fallthrough))

#endif
