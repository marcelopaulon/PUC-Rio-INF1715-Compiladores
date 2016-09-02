/* scanner for the Monga language */
/* Marcelo Paulon - 1411029 / Renan da Fonte - 1412122*/

%option noyywrap
%option nounput
%option noinput

%{
/* need this for the call to strtod/strtoul/strlen/strcpy below */
#include <string.h>
#include "monga.h"

extern MongaToken token;

static int curLine = 1;

MongaData getSymbolData(MongaSymbol symbol, char *data)
{
  MongaData mongaData;
  char *idStr;
  
  switch(symbol)
  {
    case TK_ID:
    case TK_STRING:
      idStr = malloc((strlen(data) + 1) * sizeof(char));
      if(idStr == NULL)
      {
        printf("Unable to allocate memory. Stopping.");
        exit(-1);
      }
      strcpy(idStr, data);
      mongaData.c = idStr;
      break;
    case TK_DOUBLE_NUMBER:
      mongaData.d = strtod(data, NULL);
      break;
    case TK_LONG_NUMBER:
      mongaData.l = strtoull(data, NULL, 10);
      break;
    case TK_UNKNOWN:
      printf("Unrecognized token. Stopping.\n");
      exit(-1);
    default:
      break;
  }

  return mongaData;
}

int addSymbol(MongaSymbol symbol, char *data)
{
  token.symbol = symbol;
  token.data = getSymbolData(symbol, data);
  token.line = curLine;
  return symbol;
}
%}

%%

";"			return addSymbol(TK_SEMICOLON, yytext);

"{"			return addSymbol(TK_OPEN_KEY, yytext);
"}"			return addSymbol(TK_CLOSE_KEY, yytext);
"("			return addSymbol(TK_OPEN_PARENTHESIS, yytext);
")"			return addSymbol(TK_CLOSE_PARENTHESIS, yytext);
"["			return addSymbol(TK_OPEN_BRACKET, yytext);
"]"			return addSymbol(TK_CLOSE_BRACKET, yytext);
"="			return addSymbol(TK_EQUALS, yytext);
"+"			return addSymbol(TK_PLUS, yytext);
"-"			return addSymbol(TK_MINUS, yytext);
"*"			return addSymbol(TK_ASTERISK, yytext);
"/"			return addSymbol(TK_SLASH, yytext);
"<="			return addSymbol(TK_LE, yytext);
"<"			return addSymbol(TK_LT, yytext);
">="			return addSymbol(TK_GE, yytext);
">"			return addSymbol(TK_GT, yytext);
"&&"			return addSymbol(TK_AND, yytext);
"||"			return addSymbol(TK_OR, yytext);

"if"			return addSymbol(TK_IF, yytext);
"else"			return addSymbol(TK_ELSE, yytext);

"char"			return addSymbol(TK_CHAR, yytext);
"float"			return addSymbol(TK_FLOAT, yytext);
"int"			return addSymbol(TK_INT, yytext);
"new"			return addSymbol(TK_NEW, yytext);
"return"		return addSymbol(TK_RETURN, yytext);
"void"			return addSymbol(TK_VOID, yytext);
"while"			return addSymbol(TK_WHILE, yytext);

[0-9]+ 			return addSymbol(TK_LONG_NUMBER, yytext);

[0-9]+"."[0-9]*         return addSymbol(TK_DOUBLE_NUMBER, yytext);

[A-Za-z][A-Za-z0-9]*    return addSymbol(TK_ID, yytext);

[ \t\n]+          /* eat up whitespace */

.           		addSymbol(TK_UNKNOWN, yytext);

%%
