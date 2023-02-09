%{
// 在 %{ 和 %} 中的代码会被原样照抄到生成的 lex.yy.c 文件的开头，您可以在这里书写声明与定义
#include <string.h>
int chars = 0;
int words = 0;
%}

%%
 /* 你可以在这里使用你熟悉的正则表达式来编写模式 */
 /* 你可以用C代码来指定模式匹配时对应的动作 */
 /* yytext 指针指向本次匹配的输入文本 */
 /* 左部分 [a-zA-Z]+ 为要匹配的正则表达式，
 	  右部分 { chars += strlen(yytext); words++; } 为匹配到该正则表达式后执行的动作 */
[a-zA-Z]+ { chars += strlen(yytext); words++; }

 /* 对其他所有字符，不做处理，继续执行 */
. {}
%%

int main(int argc, char **argv){
    // yylex() 是 flex 提供的词法分析例程，默认读取 stdin      
    yylex();                                                               
    printf("look, I find %d words of %d chars\n", words, chars);
    return 0;
}