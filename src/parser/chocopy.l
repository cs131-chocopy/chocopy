%option noyywrap
%option yylineno

/* The prefix for imports in c syntax */
%{
/* For Indentation */
#include <stack>
#include <iostream>
#include <cstdlib>
#include <cstdio>

#if __cplusplus > 199711L
#define register      // Deprecated in C++11.
#endif  // #if __cplusplus > 199711L

using namespace std;
std::stack<int> stack_indent;

#include "chocopy_parse.hpp"
#include "chocopy.tab.h"

int ColNum = 1;
int LineNum = 1;
#define SET_ACTION                                                           \
  yylloc.first.line = LineNum;                                               \
  yylloc.first.column = ColNum;                                              \
  ColNum += yyleng;                                                          \
  yylloc.last.line = LineNum;                                                \
  yylloc.last.column = ColNum;
%}

%x CODE

%%

%{
  /* Init indent stack_indent */
  if (stack_indent.empty()) {
    stack_indent.push(0);
  }

%}

<CODE>"True" { SET_ACTION return TOKEN_TRUE; }
<CODE>"False" { SET_ACTION return TOKEN_FALSE; }
<CODE>"None" { SET_ACTION return TOKEN_NONE; }

<CODE>"and" { SET_ACTION return TOKEN_AND; }
<CODE>"not" { SET_ACTION return TOKEN_NOT; }
<CODE>"or" { SET_ACTION return TOKEN_OR; }

<CODE>"as" { SET_ACTION return TOKEN_AS; }
<CODE>"in" { SET_ACTION return TOKEN_IN; }
<CODE>"is" { SET_ACTION return TOKEN_IS; }

<CODE>"global" { SET_ACTION return TOKEN_GLOBAL; }
<CODE>"nonlocal" { SET_ACTION return TOKEN_NONLOCAL; }

<CODE>"if" { SET_ACTION return TOKEN_IF; }
<CODE>"else" { SET_ACTION return TOKEN_ELSE; }
<CODE>"elif" { SET_ACTION return TOKEN_ELIF; }

<CODE>"for" { SET_ACTION return TOKEN_FOR; }
<CODE>"while" { SET_ACTION return TOKEN_WHILE; }
<CODE>"break" { SET_ACTION return TOKEN_BREAK; }

<CODE>"def" { SET_ACTION return TOKEN_DEF; }
<CODE>"return" { SET_ACTION return TOKEN_RETURN; }
<CODE>"pass" { SET_ACTION return TOKEN_PASS; }

<CODE>"class" { SET_ACTION return TOKEN_CLASS; }

<CODE>"+" { SET_ACTION return TOKEN_plus; }
<CODE>"-" { SET_ACTION return TOKEN_minus; }
<CODE>"*" { SET_ACTION return TOKEN_star; }
<CODE>"//" { SET_ACTION return TOKEN_slash; }
<CODE>"%" { SET_ACTION return TOKEN_percent; }

<CODE>"=" { SET_ACTION return TOKEN_equal; }

<CODE>"<=" { SET_ACTION return TOKEN_lessequal; }
<CODE>">=" { SET_ACTION return TOKEN_greaterequal; }
<CODE>"==" { SET_ACTION return TOKEN_equalequal; }
<CODE>"!=" { SET_ACTION return TOKEN_exclaimequal; }
<CODE>"<" { SET_ACTION return TOKEN_less; }
<CODE>">" { SET_ACTION return TOKEN_greater; }

<CODE>"(" { SET_ACTION return TOKEN_l_paren; }
<CODE>")" { SET_ACTION return TOKEN_r_paren; }
<CODE>"[" { SET_ACTION return TOKEN_l_square; }
<CODE>"]" { SET_ACTION return TOKEN_r_square; }
<CODE>"," { SET_ACTION return TOKEN_comma; }
<CODE>":" { SET_ACTION return TOKEN_colon; }
<CODE>"." { SET_ACTION return TOKEN_period; }
<CODE>"->" { SET_ACTION return TOKEN_rarrow; }

<CODE>[0-9]+ {
  yylval.raw_int = atoi(yytext);
  SET_ACTION
  return TOKEN_INTEGER;
}

<CODE>(assert)|(async)|(await)|(continue)|(del)|(except)|(finally)|(from)|(import)|(lambda)|(raise)|(try)|(with)|(yield) {
  LOG(ERROR) << "Error: reversed token on line " << yylineno << ": " << yytext;
  return ERROR;
}

<CODE>[a-zA-Z_][a-zA-Z_0-9]* {
  yylval.raw_str = strdup(yytext);
  SET_ACTION
  return TOKEN_IDENTIFIER;;
}

<CODE>[ \t]* { ColNum += yyleng; }
<CODE>(#.*)?(\r|(\r?\n)) {
  BEGIN INITIAL;

  SET_ACTION
  LineNum += 1, ColNum = 1;
  return TOKEN_NEWLINE;;
}

^[ \t]*#.* { }

[ \t]*[^\r\n\t ] {
  static int indent_status = 0;
  int spaces = yyleng - 1;

  LineNum = yylineno;
  ColNum = 1;

  yylloc.first.line = LineNum;
  yylloc.first.column = ColNum;
  ColNum += yyleng - 1;
  yylloc.last.line = LineNum;
  yylloc.last.column = ColNum;

  if (stack_indent.top() < spaces) {
    if (indent_status == 1) {
      return ERROR;
    }
    indent_status = 2;
    stack_indent.push(spaces);
    BEGIN CODE;
    yyless(yyleng - 1);
    return TOKEN_INDENT;
  }

  if (stack_indent.top() > spaces) {
    indent_status = 1;
    stack_indent.pop();
    yyless(0);
    return TOKEN_DEDENT;
  }

  // stack_indent.top() == spaces
  indent_status = 0;
  BEGIN CODE;
  yyless(yyleng - 1);
}

<CODE><<EOF>> {
  // LOG(WARNING) << "INITIAL";
  BEGIN INITIAL;

  SET_ACTION ColNum--;
  return TOKEN_NEWLINE;;
}

^[\t ]+ { }

(\r|(\r?\n)) { LineNum += 1, ColNum = 1; }

<<EOF>> {
  if (stack_indent.top() != 0) {
    stack_indent.pop();

    SET_ACTION
    return TOKEN_DEDENT;
  }
  return 0;
}

<CODE>(\"([ -!#-\[\]-~]|(\\(n|t|\\|\")))*\") {
  yylval.raw_str = new char[yyleng+1];
  int len = 0;

  for (int i = 1; i < yyleng-1; i++) {
    if (yytext[i] == '\\') {
      yylval.raw_str[len++] = yytext[i+1] == 'n' ? '\n' : yytext[i+1] == 't' ? '\t' : yytext[i+1] == '\\' ? '\\' : '\"';
      i++;
    } else {
      yylval.raw_str[len++] = yytext[i];
    }
  }
  yylval.raw_str[len] = '\0';

  SET_ACTION return TOKEN_STRING;
}

. {
        LOG(ERROR) << "Error: Unrecognized token on line " << yylineno << ": " << yytext;
        return ERROR;
}
<CODE>. {
        LOG(ERROR) << "Error: Unrecognized token on line " << yylineno << ": " << yytext;
        return ERROR;
}
%%

