# Programing Assignment IV 文档

在 CodeGen 中，我们需要从 Light IR 生成后端代码，LLVM IR 仅需要简单的转换指令、函数调用指定和寄存器分配就可以了。这也是为什么LLVM IR这么风靡的原因，很简单的对任意体系结构的代码生成。LLVM自己的实现可以参考[源码这里](https://github.com/llvm/llvm-project/blob/main/llvm/include/llvm/IR/IntrinsicsRISCV.td).

## 基础知识

### 标准库

定义详见 [src/cgen/stdlib/stdlib.c](../../src/cgen/stdlib/stdlib.c)，遵守 RISC-V calling convention 即可。Calling Convention 规定了函数调用参数传递方法，返回值传递方法，caller/callee saved registers. 请务必遵守，文档详见 [doc/riscv-calling.pdf](../riscv-calling.pdf).

### RISC-V Spec

定义详见 [RISCV Spec](https://riscv.org/technical/specifications/)， [RVV](https://github.com/riscv/riscv-v-spec).

也可以使用 godbolt 观察编译 C 语言得到的汇编指令来了解 RISC-V 指令

### 后端介绍

在 [include/cgen/InstGen.hpp](../../include/cgen/InstGen.hpp) 中有对寄存器，地址和值的抽象，为 `Reg`, `Addr`, `Constant`, `Label`.

在 [include/cgen/RiscVBackEnd.hpp](../../include/cgen/RiscVBackEnd.hpp) 有预先定义好的 `reg_name`, `caller_save_regs`, `callee_save_regs` 可以使用，还提供了 `RiscVBackEnd`， 可以调用这个类的方法 `emit_*` 来生成指令（你当然也可以不用或者自己另外写一份）。

#### 寄存器分配

评测不考察性能，实现 stack machine 即可。

### 内存管理

不用管理。

## 实验要求

### 代码结构

主函数为 `chocopy_cgen.cpp:main`, 会调用 `generateModuleCode` 根据 IR 生成汇编代码。

`generateModuleCode` 会调用 `backend->emit_prototype` 在 data segment 生成 ChocoPy 类的 prototype, 然后调用 `generateGlobalVarsCode` 生成全局变量。之后将调用 `CodeGen::generateFunctionCode` 在 text segment 生成代码。

在 `CodeGen::generateFunctionCode` 中生成一些代码，然后调用 `CodeGen::generateBasicBlockCode`.

`CodeGen::generateBasicBlockCode` 会遍历 BasicBlock 中的指令，调用 `CodeGen::generateInstructionCode` 生成指令汇编。也会调用 `CodeGen::generateBasicBlockPostCode` 来处理 phi 指令。

### 主要工作

修改 [chocopy_cgen.cpp](../../src/cgen/chocopy_cgen.cpp) 来实现从 IR 到 RISC-V 汇编，使得它能正确编译 ChocoPy 程序。

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
