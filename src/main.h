#ifndef __main_h__
#define __main_h__

#define ArrayCount(A) (sizeof (A) / sizeof *(A))

#define IsDelimChar(C) ((C) == '\t')
#define IsCommentChar(C) ((C) == '#')
#define IsCommandChar(C) ((C) == ':')
#define IsEvalChar(C) ((C) == '=')

#define AltDelimFor(C, A) ((C) != ArrayCount(ColWidth)-1? A: "\n")
#define DelimFor(C) AltDelimFor(C, " ")

#endif
