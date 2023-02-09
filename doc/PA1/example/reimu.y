%{
#include <stdio.h>
/* 这里是序曲 */
/* 这部分代码会被原样拷贝到生成的 .c 文件的开头 */
int yylex(void);
void yyerror(const char *s);
extern void yyrestart(FILE*); /* 如果输入参数为文件，则从文件读取 */
%}

/* 这些地方可以输入一些 bison 指令 */
/* 比如用 %start 指令指定起始符号，用 %token 定义一个 token */
%start reimu
%token REIMU

%%
/* 从这里开始，下面是解析规则 */
reimu : marisa { /* 这里写与该规则对应的处理代码 */ puts("rule1"); }
      | REIMU { /* 这里写与该规则对应的处理代码 */ puts("rule2"); }
      
/* 这种写法表示 ε —— 空输入 */
marisa : { puts("Hello!"); }

%%
/* 这里是尾声 */
/* 这部分代码会被原样拷贝到生成的 .c 文件的末尾 */

/* 手写一个简单词法分析 */
int yylex(void) {
    int c = getchar(); // 从 stdin 获取下一个字符 
    switch (c) {
      case EOF: return YYEOF;
      case 'R': return REIMU;
      default: return YYerror; // 使 Bison 报错
    }
}

void yyerror(const char *s) {
    fprintf(stderr, "syntax error");
}

int main(void) {
    if (yyparse()) // 启动解析
        fprintf(stderr, "syntax error\n");
    return 0;
}