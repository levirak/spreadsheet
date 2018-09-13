#ifndef __main_h__
#define __main_h__

#define ArrayCount(A) (sizeof (A) / sizeof *(A))

#define DELIM_CHAR   '\t'
#define COMMENT_CHAR '#'
#define COMMAND_CHAR ':'
#define EVAL_CHAR    '='

#define IsDelimChar(C) ((C) == DELIM_CHAR)
#define IsCommentChar(C) ((C) == COMMENT_CHAR)
#define IsCommandChar(C) ((C) == COMMAND_CHAR)
#define IsEvalChar(C) ((C) == EVAL_CHAR)

#define AltDelimFor(C, A) ((C) != ArrayCount(ColWidth)-1? A: "\n")
#define DelimFor(C) AltDelimFor(C, " ")

#endif
