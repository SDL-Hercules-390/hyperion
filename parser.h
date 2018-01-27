/* PARSER.H     (C) Copyright Roger Bowler, 1999-2012                */
/*              Hercules Simple parameter parser                     */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#if !defined( _PARSER_H_ )
#define _PARSER_H_

#include "hercules.h"

#define  MAX_PARSER_STRLEN           79
#define  _PARSER_STR_TYPE( len )     "%" QSTR( len ) "s"
#define  PARSER_STR_TYPE             _PARSER_STR_TYPE( MAX_PARSER_STRLEN )

typedef struct _parser
{
    char *key;
    char *fmt;
} PARSER;

PAR_DLL_IMPORT int parser( PARSER *, char *, void * );

#endif /* !defined( _PARSER_H_ ) */
