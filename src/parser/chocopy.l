%option noyywrap
%option yylineno

/* The prefix for imports in c syntax */
%{
/* For Indentation */
#include <stack>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <string>

#define register      // Deprecated in C++11.

std::string hello = "Hello World"; // just a example

#include "chocopy_parse.hpp"
#include "chocopy.tab.h"
%}

%%

%{
  /* You can write some initial codes here */
  if (false) {
    std::cout << hello;
  }
%}

"+" { return TOKEN_plus; }

[0-9]+ {
  yylval.raw_int = atoi(yytext);
  return TOKEN_INTEGER;
}

\n {
  return TOKEN_NEWLINE;
}

. {
        LOG(ERROR) << "Error: Unrecognized token on line " << yylineno << ": " << yytext;
        return YYerror;
}
%%

