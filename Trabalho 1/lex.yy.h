#if !defined(LEXYY_H)
#define LEXYY_H

#define DEBUG 0

#define MONGA_UNION_TYPE MongaData

extern int yylex(void);

typedef enum mongaSymbol {
  TK_LE = 256,
  TK_GE,
  TK_AND,
  TK_OR,
  TK_IF,
  TK_ELSE,
  TK_CHAR,
  TK_FLOAT,
  TK_INT,
  TK_NEW,
  TK_RETURN,
  TK_VOID,
  TK_WHILE,
  TK_ID,
  TK_UNKNOWN,
  TK_DOUBLE_NUMBER,
  TK_LONG_NUMBER,
  TK_STRING,
  TK_EQ
} MongaSymbol;

typedef union mongaData {
  unsigned long long int l;
  double d;
  char *c;
} MongaData;

typedef struct mongaToken {
  MongaSymbol symbol;
  MONGA_UNION_TYPE data;
  int line;
} MongaToken;

#endif
