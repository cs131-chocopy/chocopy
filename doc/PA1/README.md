# Programing Assignment I 文档

## 基础知识

在本次实验中我们将用到 flex, Bison 和以 Python3.6 为基础改编的 ChocoPy 语言。

### ChocoPy 语法和词法

请参考[ChocoPy Language Reference](../chocopy_language_reference.pdf)。

### flex

flex 是一个生成词法分析器的工具。利用 flex，我们只需提供词法的正则表达式，就可自动生成对应的C代码。整个流程如下图：

![](http://alumni.cs.ucr.edu/~lgao/teaching/Img/flex.jpg)

1. flex 从输入文件读取词法扫描器的规则，生成 C 代码源文件`lex.yy.c`。
2. 编译 `lex.yy.c` 并与 `-lfl` 库链接，以生成可执行的 `a.out`。
3. `a.out` 分析其输入流，将其转换为一系列token。

我们以一个简单的单词数量统计的程序 wc.l 为例:

```c
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
```

使用 flex 生成 lex.yy.c

```bash
$ flex wc.l 
$ gcc lex.yy.c -lfl
$ ./a.out 
hello world
^D
look, I find 2 words of 10 chars
```

*注: 在以 stdin 为输入时，需要按下 ctrl+D 以退出*

至此，你已经成功使用 flex 完成了一个简单的词法分析器！

### Bison 简单使用

本次实验需要在已完成的 flex 词法分析器的基础上，进一步使用 Bison 完成语法分析器。

Bison 是一款解析器生成器（parser generator），它可以将 LALR 文法转换成可编译的 C 代码，从而大大减轻程序员手动设计解析器的负担。
Bison 是 GNU 对早期 Unix 的 Yacc 工具的一个重新实现，所以文件扩展名为 `.y`。（Yacc 的意思是 Yet Another Compiler Compiler）

每个 Bison 文件由 `%%` 分成三部分。

```c
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
```

另外有一些值得注意的点：

1. Bison 传统上将 token 用大写单词表示，将 symbol 用小写字母表示。
2. Bison 能且只能生成解析器源代码（一个 `.c` 文件），并且入口是 `yyparse`，所以为了让程序能跑起来，你需要手动提供 `main` 函数（但不一定要在 `.y` 文件中——你懂“链接”是什么，对吧？）。
3. Bison 不能检测你的 action code 是否正确——它只能检测文法的部分错误，其他代码都是原样粘贴到 `.c` 文件中。
4. Bison 需要你提供一个 `yylex` 来获取下一个 token。
5. Bison 需要你提供一个 `yyerror` 来提供合适的报错机制。
6. Bison 如果提供全局变量 `yydebug` 可以给出接收过程输出。

顺便提一嘴，上面这个 `.y` 是可以工作的——尽管它只能接受两个字符串。把上面这段代码保存为 `reimu.y`，执行如下命令来构建这个程序：

```shell
$ bison reimu.y
$ gcc reimu.tab.c
$ ./a.out
R<-- 不要回车在这里按 Ctrl-D
rule2
$ ./a.out
<-- 不要回车在这里按 Ctrl-D
Hello!
rule1
$ ./a.out
blablabla <-- 回车或者 Ctrl-D
syntax error <-- 发现了错误
```

于是我们验证了上述代码的确识别了该文法定义的语言 `{ "", "R" }`。

### Bison 和 flex 的关系

聪明的你应该发现了，我们这里手写了一个 `yylex` 函数作为词法分析器。
而之前我们正好使用 flex 自动生成了一个词法分析器。如何让这两者协同工作呢？
特别是，我们需要在这两者之间共享 token 定义和一些数据，难道要手动维护吗？当然不用！
下面我们用一个四则运算计算器来简单介绍如何让 Bison 和 flex 协同工作——重点是如何维护解析器状态、`YYSTYPE` 和头文件的生成。

首先，我们必须明白，整个工作流程中，Bison 是占据主导地位的，
而 flex 仅仅是一个辅助工具，仅用来生成 `yylex` 函数。因此，最好先写 `.y` 文件。

```c
/* calc.y */
%{
#include <stdio.h>
int yylex(void);
void yyerror(const char *s);
%}

%token RET
%token <num> NUMBER
%token <op> ADDOP MULOP LPAREN RPAREN
%type <num> top line expr term factor

%start top

%union {
    char   op;
    double num;
}

%%

top : top line {}
    | {}

line : expr RET { printf(" = %f\n", $1); }

expr : term { $$ = $1; }
    | expr ADDOP term {
        switch ($2) {
            case '+': $$ = $1 + $3; break;
            case '-': $$ = $1 - $3; break;
        }
    }

term : factor { $$ = $1; }
    | term MULOP factor {
        switch ($2) {
            case '*': $$ = $1 * $3; break;
            case '/': $$ = $1 / $3; break; // 这里会出什么问题？
        }
    }

factor : LPAREN expr RPAREN { $$ = $2; }
    | NUMBER { $$ = $1; }

%%

void yyerror(const char *s) {
    fprintf(stderr, "%s\n", s);
}
```

```c
/* calc.l */
%option noyywrap

%{
/* 引入 calc.y 定义的 token */
#include "calc.tab.h"
%}

%%

\( { return LPAREN; }
\) { return RPAREN; }
"+"|"-" { yylval.op = yytext[0]; return ADDOP; }
"*"|"/" { yylval.op = yytext[0]; return MULOP; }
[0-9]+|[0-9]+\.[0-9]*|[0-9]*\.[0-9]+ { yylval.num = atof(yytext); return NUMBER; }
" "|\t {  }
\r\n|\n|\r { return RET; }

%%
```

最后，我们补充一个 `calc_driver.c` 来提供 `main` 函数。

```c
int yyparse();

int main() {
    yyparse();
    return 0;
}
```

使用如下命令构建并测试程序：

```shell
$ bison -d calc.y 
   (生成 calc.tab.c 和 calc.tab.h。如果不给出 -d 参数，则不会生成 .h 文件。)
$ flex calc.l
   (生成 lex.yy.c)
$ gcc lex.yy.c calc.tab.c calc_driver.c -o calc
$ ./calc
1+1
 = 1.000000
2*(1+1)
 = 4.000000
2*1+1
 = 3.000000
```

如果你复制粘贴了上述程序，可能会觉得很神奇，并且有些地方看不懂。下面就详细讲解上面新出现的各种构造。

* `YYSTYPE`: 在 Bison 解析过程中，每个 symbol 最终都对应到一个语义值上。或者说，在 parse tree 上，每个节点都对应一个语义值，这个值的类型是 `YYSTYPE`。
  `YYSTYPE` 的具体内容是由 `%union` 构造指出的。上面的例子中，

  ```c
  %union {
    char   op;
    double num;
  }
  ```

  会生成类似这样的代码

  ```c
  typedef union YYSTYPE {
    char op;
    double num;
  } YYSTYPE;
  ```

  为什么使用 `union` 呢？因为不同节点可能需要不同类型的语义值。比如，上面的例子中，我们希望 `ADDOP` 的值是 `char` 类型，而 `NUMBER` 应该是 `double` 类型的。

* `$$` 和 `$1`, `$2`, `$3`, ...：现在我们来看如何从已有的值推出当前节点归约后应有的值。以加法为例：

  ```c
  term : factor { $$ = $1; }
    | term MULOP factor {
        switch ($2) {
            case '*': $$ = $1 * $3; break;
            case '/': $$ = $1 / $3; break; // 这里会出什么问题？
        }
    }
  ```

  其实很好理解。当前节点使用 `$$` 代表，而已解析的节点则是从左到右依次编号，称作 `$1`, `$2`, `$3`...

* `%type <>` 和 `%token <>`：注意，我们上面可没有写 `$1.num` 或者 `$2.op` 哦！那么 Bison 是怎么知道应该用 `union` 的哪部分值的呢？其秘诀就在文件一开始的 `%type`
  和 `%token` 上。

  例如，`term` 应该使用 `num` 部分，那么我们就写

  ```c
  %type <num> term
  ```

  这样，以后用 `$` 去取某个值的时候，Bison 就能自动生成类似 `stack[i].num` 这样的代码了。

  `%token<>` 见下一条。

* `%token`：当我们用 `%token` 声明一个 token 时，这个 token 就会导出到 `.h` 中，可以在 C 代码中直接使用（注意 token 名千万不要和别的东西冲突！），供 flex
  使用。`%token <op> ADDOP` 与之类似，但顺便也将 `ADDOP` 传递给 `%type`，这样一行代码相当于两行代码，岂不是很赚。

* `yylval`：这时候我们可以打开 `.h` 文件，看看里面有什么。除了 token 定义，最末尾还有一个 `extern YYSTYPE yylval;` 。这个变量我们上面已经使用了，通过这个变量，我们就可以在 lexer 设置某个 token 的值。

呼……说了这么多，现在回头看看上面的代码，应该可以完全看懂了吧！
这时候你可能才意识到为什么 flex 生成的分析器入口是 `yylex`，
因为这个函数就是 Bison 专门让程序员自己填的，作为一种扩展机制。
另外，Bison 生成的变量和函数名通常都带有 `yy` 前缀。

最后还得提一下，尽管上面所讲已经足够应付很大一部分解析需求了，
但是 Bison 还有一些高级功能，比如自动处理运算符的优先级和结合性（于是我们就不需要手动把 `expr` 拆成 `factor`, `term` 了）。
这部分功能，就留给同学们自己去探索吧！

## 实验要求

本实验的输出类似 Language Server Protocol，
如你们在 web 上测试的情况可知，这种 JSON 的传输协议很适合作为前后端现实的交互接口，
VSCode等IDE也使用 LSP 进行前端高亮。

本次实验需要各位同学根据 ChocoPy 的词法和语法补全 [chocopy.l](./src/parser/chocopy.l) 以及 [chocopy.y](./src/parser/chocopy.l) 文件，
完成完整的语法分析器。不用担心，我们已经为你写好了用 AST 生成 JSON 的部分。你只需要使用 flex 和 Bison 生成 AST 即可。

### 主要工作

1. 了解 Bison 基础知识和理解 ChocoPy 语法（重在了解如何将文法产生式转换为 Bison 语句）
2. 阅读 `./src/parser/chocopy_parse.cpp`，对应头文件 `./include/parser/chocopy_parse.hpp`（重在理解分析树如何生成）
3. 了解 Bison 与 flex 之间是如何协同工作的
4. 补全 `src/parser/chocopy.y` 文件和 `chocopy.l` 文件

### 提示

文本输入：

```py
a: int = 1
print(a)
```

则输出结果应为：

```json
{
  "kind": "Program",
  "location": [1, 1, 2, 9],
  "declarations": [
    {
      "kind": "VarDef",
      "location": [1, 1, 1, 11],
      "value": {
        "kind": "IntegerLiteral",
        "location": [1, 10, 1, 11],
        "value": 1
      },
      "var": {
        "identifier": {
          "kind": "Identifier",
          "location": [1, 1, 1, 2],
          "name": "a"
        },
        "kind": "TypedVar",
        "location": [1, 1, 1, 7],
        "type": {
          "className": "int",
          "kind": "ClassType",
          "location": [1, 4, 1, 7]
        }
      }
    }
  ],
  "statements": [
    {
      "expr": {
        "args": [
          {
            "kind": "Identifier",
            "location": [2, 7, 2, 8],
            "name": "a"
          }
        ],
        "function": {
          "kind": "Identifier",
          "location": [2, 1, 2, 6],
          "name": "print"
        },
        "kind": "CallExpr",
        "location": [2, 1, 2, 9]
      },
      "kind": "ExprStmt",
      "location": [2, 1, 2, 9]
    }
  ],
  "errors": {
    "errors": [],
    "kind": "Errors",
    "location": [0, 0, 0, 0]
  }
}
```

如果有词法错误，在 `ROOT->errors->compiler_errors` 中添加 `parser::CompilerErr` 即可。
`chocopy.y` 中的 `yyerror()` 已经做了这件事。

评测程序不会检查错误信息与个数，找到一个不可接收的程序直接报错返回。

**具体的需识别token参考[chocopy.y](./src/parser/chocopy.y)，需要实现的抽象语法树参考[chocopy_parse.hpp](./include/parser/chocopy_parse.hpp)**

### 编译、运行和验证

* 编译

  若编译成功，则将在 `./[build_dir]/` 下生成 `parser`。

* 运行

  ```shell
  $ cd chocopy
  $ ./build/parser               # 交互式使用（不进行输入重定向）
  <在这里输入 ChocoPy代码，如果遇到了错误，将程序将报错并退出。>
  <输入完成后按 ^D 结束输入，此时程序将输出解析json。>
  $ ./build/parser test.py  # 不使用重定向，直接从 test.py 中读入
  ```

* 验证

  本次试验测试案例较多，为此我们将这些测试分为两类：

    1. sample: 这部分测试均比较简单且单纯，适合开发时调试。
    2. fuzz: 由fuzzer生成的正确的python文件，此项不予开源。
    3. student: 这部分由同学提供。

  我们使用 `check.py` 进行评分。`check.py` 会将 `parser` 的生成结果和助教提供的 `*.py.ast` 进行比较。

  ```shell
  $ ./check.py --pa 1
  ```

  **请注意助教提供的`testcase`并不能涵盖全部的测试情况，完成此部分仅能拿到基础分，请自行设计自己的`testcase`进行测试。**

### 提供可用的测试用例

对于每个学生，你需要在资源库的根目录下创建一个名为 `tests/pa1/student/` 的文件夹，
并放置 10 个有意义的 `*.py` 测试案例，
其中 5 个将通过所有的编译，
另外 5 个将不通过编译，来测试你代码的错误报告。

你的测试案例将被用来评估其他人的代码。

### 评分

1. 基本测试样例[70pts]
2. Fuzzer 测试[5pts]
3. Student 测试[5pts]
4. 提供 TestCase[5pts]
5. Code interview[15pts]
