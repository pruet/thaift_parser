/* Copyright (c) 2005, 2011, Oracle and/or its affiliates
   Copyright (c) 2016 Pruet Boonma <pruet@eng.cmu.ac.th>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#include "my_config.h"
#include <string.h> 
#include <log.h>
#include <fts0tokenize.h>
#include <thai/thbrk.h>
#include <thai/thwchar.h>
#include <thai/thwbrk.h>

#if !defined(__attribute__) && (defined(__cplusplus) || !defined(__GNUC__)  || __GNUC__ == 2 && __GNUC_MINOR__ < 8)
#define __attribute__(A)
#endif

/*
  Thai full-text parser plugin that acts as a replacement for the
  built-in full-text parser:
  - All non-whitespace characters are significant and are interpreted as
   "word characters."
  - Whitespace characters are space, tab, CR, LF.
  - There is no minimum word length.  Non-whitespace sequences of one
    character or longer are words.
  - Support both natural and boolean search.
  - This is 1-gram, with very small dictionary (one which included with libthai).

  Test cases provided by Vee Satayamas <vsatayamas@gmail.com>
*/


/*
  Initialize the parser plugin at server start or plugin installation.

  SYNOPSIS
    thai_parser_plugin_init()

  DESCRIPTION
    Does nothing.

  RETURN VALUE
    0                    success
    1                    failure (cannot happen)
*/

static int thai_parser_plugin_init(void *arg __attribute__((unused)))
{
  /* 
    TODO/
    This is not pretty, but I don't know how to make it better.
    Need to consult P'Thep
  */
  int length;
  const char *str = "ÇÑ¹¹Õé½¹äÁèµ¡";
  int *pos;

  length = strlen(str);
  pos= (int *)malloc(sizeof(int) * length);
  if (NULL == pos) {
		sql_print_error("Thaift: thai_parse_plugin_init() failed: out of memory.");
    return(1);
  }
  //TO th_wbrk?
  if(th_brk((uchar *)str, pos, length) == 0) {
		sql_print_error("Thaift: thai_parse_plugin_init() failed: Can't load libthai.");
    return(1);
  }
  return(0);
}


/*
  Terminate the parser plugin at server shutdown or plugin deinstallation.

  SYNOPSIS
    thai_parser_plugin_deinit()
    Does nothing.

  RETURN VALUE
    0                    success
    1                    failure (cannot happen)

*/

static int thai_parser_plugin_deinit(void *arg __attribute__((unused)))
{
  return(0);
}


/*
  Initialize the parser on the first use in the query

  SYNOPSIS
    thai_parser_init()

  DESCRIPTION
    Does nothing.

  RETURN VALUE
    0                    success
    1                    failure (cannot happen)
*/

static int thai_parser_init(MYSQL_FTPARSER_PARAM *param
                              __attribute__((unused)))
{
  return(0);
}


/*
  Terminate the parser at the end of the query

  SYNOPSIS
    thai_parser_deinit()

  DESCRIPTION
    Does nothing.

  RETURN VALUE
    0                    success
    1                    failure (cannot happen)
*/

static int thai_parser_deinit(MYSQL_FTPARSER_PARAM *param
                                __attribute__((unused)))
{
  return(0);
}


/*
  Pass a word back to the server.

  SYNOPSIS
    add_word()
      param              parsing context of the plugin
      word               a word
      len                word length
      bool_info          word's attributes

  DESCRIPTION
    Fill in boolean metadata for the word (if parsing in boolean mode)
    and pass the word to the server.  The server adds the word to
    a full-text index when parsing for indexing, or adds the word to
    the list of search terms when parsing a search string.
*/

static int add_word(MYSQL_FTPARSER_PARAM *param, const char *word,
                    size_t len, MYSQL_FTPARSER_BOOLEAN_INFO *bool_info)
{
  return param->mysql_add_word(param,  const_cast<char*>(word),
                               len, bool_info);
}

/*
  Parse a character chunk (i.e., string surrounded by whitespaces).
  If the chunk is in English, it will be a single word. If it is in
  Thai, it may contains multiple words.

  SYNOPSIS
    thai_parser_parse()
      param              parsing context
      str                a chunk


  DESCRIPTION
    This function process a single word before sending to the
    index engine. It will convert the character set of the word
    to TIS620. This step is required by libthai's wordbreak
    function. Then it will check whether the word is in English
    or Thai. If former, the word will be sent to indexing engine.
    For Thai, it will need to be broken into words, before 
    being sent to the indexing engine.
*/

static int thai_parse(MYSQL_FTPARSER_PARAM *param, const char *str,
                      int length, MYSQL_FTPARSER_BOOLEAN_INFO *bool_info)
{
  int i, numBytePerChar, numCut;
  int *pos;
  uchar *toStr;
  uint *error;
 
  toStr= (uchar *) malloc(sizeof(uchar)
        * param->length
        * my_charset_tis620_thai_ci.mbmaxlen);
  if (NULL == toStr) {
		sql_print_error("Thaift: thai_parse() failed: out of memory.");
    return(1);
  }
  error= (uint *) malloc(sizeof(uint) * param->length);
  if (NULL == error) {
    free(toStr);
		sql_print_error("Thaift: thai_parse() failed: out of memory.");
    return(1);
  }
  pos= (int *)malloc(sizeof(int) * length);
  if (NULL == pos) {
    free(toStr);
    free(error);
		sql_print_error("Thaift: thai_parse() failed: out of memory.");
    return(1);
  }
  numBytePerChar= param->cs->mbmaxlen;

  /* convert to tis620, make it compatible with libthai */
  my_convert((char *)toStr, length, &my_charset_tis620_thai_ci, 
             str, length, param->cs,error);
 
  /*
    Check if this is an english word, if so, add to
    index directly
  */ 
  /* TODO: any better idea than this? */
  if(toStr[0] < 161) {
    add_word(param, str, length, bool_info);  
  } else {
    /* This is Thai word/pharse */
    /* find words boundary */ 
    /* seem like numCut = sizeof(pos) - 1 ? */
    numCut= th_brk (toStr, pos, length);
    
    /* split word, add to index */
    for(i= 0; i <= numCut; i++) {
      if (0 == i) { /* first word */
        add_word(param, str, pos[i] * numBytePerChar, bool_info);  
      } else {
        add_word(param,
                str + (pos[i - 1] * numBytePerChar),
                (pos[i] - pos[i - 1]) * numBytePerChar,
                bool_info);  
      }
    }
  }
  
  free(pos);
  free(error);
  free(toStr);
  return(0); 
}

/*
  Parse a document or a search query.

  SYNOPSIS
    thai_parser_parse()
      param              parsing context

  DESCRIPTION
    This is the main plugin function which is called to parse
    a document or a search query. The call mode is set in
    param->mode.  This function simply splits the text into words
    and passes every word to thai_parse function.

  RETURN VALUE
    0                    success
    1                    failure (cannot happen)
*/

static int thai_parser_parse(MYSQL_FTPARSER_PARAM *param)
{
  FT_WORD word = {NULL, 0, 0};
  int ret = 0;
  uchar **start= reinterpret_cast<uchar**>(&param->doc);
  uchar *end= *start + param->length;
  MYSQL_FTPARSER_BOOLEAN_INFO	bool_info =
  { FT_TOKEN_WORD, 0, 0, 0, 0, 0, ' ', 0};
  // split string into token, we need this to detect Thai/English 
  while (fts_get_word(param->cs, start, end, &word, &bool_info)) {
    if (FT_TOKEN_WORD == bool_info.type) {
      ret= thai_parse(param, (char *) word.pos, word.len, &bool_info);
      //FIXME it possible that only partial of param->doc will be
      // indexed 
      if(ret) return(1); // error
    }
  } 
  return(0);
}

/*
  Plugin type-specific descriptor
*/

static struct st_mysql_ftparser thai_parser_descriptor=
{
  MYSQL_FTPARSER_INTERFACE_VERSION, /* interface version      */
  thai_parser_parse,              /* parsing function       */
  thai_parser_init,               /* parser init function   */
  thai_parser_deinit              /* parser deinit function */
};
/*
  Plugin library descriptor
*/

mysql_declare_plugin(thaift_parser)
{
  MYSQL_FTPARSER_PLUGIN,      /* type                            */
  &thai_parser_descriptor,  /* descriptor                      */
  "thaift_parser",            /* name                            */
  "Pruet Boonma",              /* author                          */
  "Thai Full-Text Parser",  /* description                     */
  PLUGIN_LICENSE_GPL,
  thai_parser_plugin_init,  /* init function (when loaded)     */
  thai_parser_plugin_deinit,/* deinit function (when unloaded) */
  0x0001,                     /* version                         */
  NULL,              /* status variables                */
  NULL,    /* system variables                */
  NULL,
  0,
}
mysql_declare_plugin_end;