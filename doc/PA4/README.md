# Programing Assignment IV 文档

在 CodeGen 中，我们需要从 Light IR 生成后端代码，LLVM IR 仅需要简单的转换指令、函数调用指定和寄存器分配就可以了。这也是为什么 LLVM IR 这么风靡的原因，很简单的对任意体系结构的代码生成。LLVM 自己的实现可以参考[源码这里](https://github.com/llvm/llvm-project/blob/main/llvm/include/llvm/IR/IntrinsicsRISCV.td).

## 基础知识

### 标准库

定义详见 [src/cgen/stdlib/stdlib.c](../../src/cgen/stdlib/stdlib.c)，遵守 RISC-V calling convention 即可。Calling Convention 规定了函数调用参数传递方法，返回值传递方法，caller/callee saved registers. 请务必遵守，文档详见 [doc/riscv-calling.pdf](../riscv-calling.pdf).

### RISC-V Spec

定义详见 [RISCV Spec](https://riscv.org/technical/specifications/)， [RVV](https://github.com/riscv/riscv-v-spec) [riscvbooks(chinese)](http://riscvbook.com/chinese/RISC-V-Reader-Chinese-v2p1.pdf).

也可以使用 godbolt 观察编译 C 语言得到的汇编指令来了解 RISC-V 指令

### 后端介绍

在 [include/cgen/InstGen.hpp](../../include/cgen/InstGen.hpp) 中有对寄存器，地址和值的抽象，为 `Reg`, `Addr`, `Constant`, `Label`.

在 [include/cgen/RiscVBackEnd.hpp](../../include/cgen/RiscVBackEnd.hpp) 有预先定义好的 `reg_name`, `caller_save_regs`, `callee_save_regs` 可以使用，还提供了 `RiscVBackEnd`， 可以调用这个类的方法 `emit_*` 来生成指令（你当然也可以不用或者自己另外写一份）。

#### 寄存器分配

只需要保证你遵守基本的 RISCV calling convention。因此你可以把所有变量都存到内存中。

### 内存管理

不用管理。

## 实验要求

### 代码结构

主函数为 `chocopy_cgen.cpp:main`, 会调用 `generateModuleCode` 根据 IR 生成汇编代码。

`generateModuleCode` 会调用 `backend->emit_prototype` 在 data segment 生成 ChocoPy 类的 prototype, 然后调用 `generateGlobalVarsCode` 生成全局变量。之后将调用 `CodeGen::generateFunctionCode` 在 text segment 生成代码。

在 `CodeGen::generateFunctionCode` 中生成一些代码，然后调用 `CodeGen::generateBasicBlockCode`.

`CodeGen::generateBasicBlockCode` 会遍历 BasicBlock 中的指令，调用 `CodeGen::generateInstructionCode` 生成指令汇编。也会调用 `CodeGen::generateBasicBlockPostCode` 来处理 phi 指令。

### 主要工作

修改 [chocopy_cgen.cpp](../../src/cgen/chocopy_cgen.cpp) 来实现从 IR 到 RISC-V 汇编，使得它能正确编译 ChocoPy 程序。我们推荐你实现一个类 Stack Machine (你应该在理论课上学过)。从 LLVM IR 转换到以 RISC-V 实现的类 Stack Machine 是非常容易的，分享一个大概思路：

#### 把 LLVM IR 的变量转换成 RISC-V 类 Stack Machine

在内存中约定一块地方，专门用于存放单赋值变量，使用数据结构(`std::map`)维护内存地址和单赋值变量的关系。

#### 把 LLVM IR 的一般语句转换成 RISC-V 类 Stack Machine

每次需要执行语句时，查询涉及到的单赋值变量的地址，`lw`到寄存器中。计算完成后，`sw`到相应位置。

#### 把 LLVM IR 中的 Phi 函数转换成 RISC-V 类 Stack Machine

使用`CodeGen::generateBasicBlockPostCode`时，会在涉及到的 `BasicBlock` 的某位加上一段指令，将 phi 函数的结果存好。

```c
int foo(int a, int b) {
  if (a > b) return a + b + 2;
  else return a * b * 17;
}
```

```llvm
define dso_local i32 @foo(i32 %0, i32 %1) {
  %3 = icmp sgt i32 %0, %1
  br i1 %3, label %4, label %7

4:                                                ; preds = %2
  %5 = add i32 %0, %1
  %6 = add i32 %5, 2
  br label %10

7:                                                ; preds = %2
  %8 = mul i32 %0, %1
  %9 = mul i32 %8, 17
  br label %10

10:                                               ; preds = %7, %4
  %.0 = phi i32 [ %6, %4 ], [ %9, %7 ]
  ret i32 %.0
}
```

那么`CodeGen::generateBasicBlockPostCode`会在`4`和`7`对应的`BasicBlock`中把所需要的`%6`和`%9`存在你定义的地方。

#### 把 LLVM IR 中的函数调用转换成 RISC-V 类 Stack Machine

Stack Machine 的精髓在于你在内存中约定一个起点，然后拥有一个往下生长(内存减小方向)的栈。函数调用所需的参数根据 calling convention 存到对应的寄存器。注意 caller 和 callee 分别负责需要保存的寄存器。


### 编译、运行和验证

* 编译

  若编译成功，则将在 `./[build_dir]/` 下生成 `cgen` 命令。

* 运行

  本次实验的 `cgen` 命令使用命令行参数来完成编译和运行。

  ```shell
  $ cd chocopy
  $ ./build/cgen test.py -run # 生成可执行文件并执行
  $ ./build/cgen test.py -emit # 输出 IR
  $ ./build/cgen test.py -assem # 输出汇编
  ```

* 验证

  本次试验测试案例较多，为此我们将这些测试分为两类：

    1. sample: 这部分测试均比较简单且单纯，适合开发时调试。
    2. fuzz: 由 fuzzer 生成的 Python 文件。
    3. student: 这部分由同学提供。

  我们使用 `check.py` 进行评分。`check.py` 会将 `cgen` 的生成汇编的运行结果和助教提供的 `*.py.ast.typed.s.result` 进行比较。

  ```shell
  cd chocopy/tests
  ./check.py --pa 4
  ```

  **请注意助教提供的 `testcase` 并不能涵盖全部的测试情况，完成此部分仅能拿到基础分，请自行设计自己的 `testcase` 进行测试。**

### 提供可用的测试用例

每组学生需要在 `tests/pa4/student/` 放置至少 4 个有意义的 `*.py` 测试案例，输出以 `.py.ast.typed.s.result` 结尾。

你的测试案例将被用来评估其他人的代码，你可以降低别人的分数。

### 评分

1. 基本测试样例[60pts]
2. Fuzzer 测试[5pts]
3. Student 测试[10pts]
4. 提供 Test Case[5pts]
5. Code interview[20pts]
