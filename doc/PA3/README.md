# Programing Assignment III 文档

本次实验是组队实验，请仔细阅读组队要求，并合理进行分工合作。
本实验中需要使用 visitor pattern 实现生成 IR. 实现需要参考 ChocoPy 的 Operational Semantic 部分。

注意：组队实验意味着合作，但是小组间的交流是受限的，且严格**禁止**代码的共享。
除此之外，如果小组和其它组进行了交流，必须在根目录 `README.md` 中记录下来交流的小组和你们之间交流内容。
## 学习LLVM IR

只有在你对 LLVM IR 相对熟悉后你才能顺利完成这个 Project

- Top-Down Learning: https://github.com/Evian-Zhang/llvm-ir-tutorial

- Bottom-up Learning: https://godbolt.org/z/PKbh9zcEx

## 基础知识

**务必确保你对 LLVM IR 有一定了解后再继续以下内容。**Light IR 是由 [Yiwei Yang](https://github.com/victoryang00) 开发的一套简化 LLVM IR 生成框架。我们不会 judge 你生成的 LLVM IR，我们会将 LLVM IR 其编译成可执行文件，通过比较执行结果来评测你的编译器是否正确。

完成了这一部分，你的 ChocoPy 编译器将具备了从 source code 编译到 LLVM IR (如其名，一种中间表示形式) 的能力。而 LLVM Project 可以将你的 LLVM IR 编译到其所支持的平台上的可执行文件的能力。因此，得益于 LLVM Project 的能力，你的 ChocoPy 编译器在这一阶段后将成为一款具备跨平台的、易扩展的、高效率的编译器！祝你好运。


### 怎么插入一条指令

在 LLVM IR 中，basic block 是一个顺序执行的指令段，中间没有跳转语句，
也就是说如果程序执行了 basic block 的第一个指令，程序就会按顺序执行 basic block 的所有指令。

我们可以用 `BasicBlock::create` 创建一个 basic block, 
然后将 `builder->set_insert_point(...)` 指向这个 basic block,
之后使用 `builder->create_xxx` 来插入指令。

### Light IR 类型设计

在 LightIR 上有 LLVM 的 primitive type: i8/i32/ptr. 完整的类型定义在 `Type.hpp` 中。`Type` 可以通过工厂模式 `::get()` 得到。

ChocoPy 中的类对应到 `include/ir-optimizer/Class.hpp` 中的 `lightir::Class`. 构造后会自动生成 LLVM IR 中的 struct 类型。
可以参考 `LightWalker::LightWalker` 中的代码了解如何生成 `Class`.

在 LightIR 中没有单独实现 struct 类型，但是有的时候我们需要 struct 类型（比如实现嵌套函数的时候）。`lightir::Class` 中的 `anon_` 指示这个 `Class` 是表示 ChocoPy 中的类还是一个 struct 类型。

`FunctionType` 包含 argument/return 的类型信息，还有函数的 basicblock.

`PtrType` 是指针类型，我们使用了 LLVM IR 的 opaque pointer 特性，这意味着指针在输出的 IR 文件中只会为 `ptr`，而不会是 `i8*` 等。
使用 opaque pointer 的好处主要是可以减少使用 `bitcast` 指令，具体可以阅读这篇[文章](https://llvm.org/docs/OpaquePointers.html).
ChocoPy 中的 list 在 LightIR 中表示为一个特殊的 `Class`，具体见 `LightWalker::LightWalker` 中 `list_class` 的初始化过程。

每个 `Value` 都会有一个 `Type`.

#### Light IR Constant/Global Variable

Constant 和 Global Variable 都会被定义在 `Constant*` 或 `GlobalVariable` 中，前者可以作为后者的初始化工具。在声明以后，会在 header 部分给出定义。

```cpp
int id = get_const_type_id();
auto c = ConstantStr::get("SOME STRING", id, module.get());
auto g =
  GlobalVariable::create("const_" + std::to_string(id), module.get(), c);
```

#### Light IR Function

##### 普通 Function

定义

```cpp
auto func_type = FunctionType::get(return_type, parameter_type_list);
auto function Function::create(func_type, name, module.get());

// example
print_fun = Function::create(
  FunctionType::get(void_type, {ptr_obj_type}),
  "print",
  module.get()
);
```

下面的代码示范了怎么设置 builder 让它在 function 内部写入代码。

```cpp
auto saved_bb = builder->get_insert_block();
auto bb = BasicBlock::create(module.get(), "", func);
builder->set_insert_point(bb);
builder->create... // the instruction is created in the function
...
// finish function generation, restore the basicblock pointer in builder 
builder->set_insert_point(saved_bb);
```

##### 嵌套 Function

ChocoPy 中的嵌套函数类似 C++ 的 `[&](){}`, 具体来说，

1. 如果函数中读取某个外部变量，不需要 `nonlocal/global` 显式声明，
2. 如果函数中写了某个外部变量，需要 `nonlocal/global` 声明。

Clang 实现 lambda function 的方法是 struct + operator() 仿函数（试试在 godbolt 里用 Clang 编译一段有 lambda function 的代码，看看它在 LLVM IR 里是什么样的）。

在 ChocoPy 中也可以用类似的方法实现。在构造 `lightir::Class` 时传入 `anon_=1` 参数，让这个类作为 `struct` 使用。

不过你可以任意选择如何实现嵌套函数，只要能通过测试 :)

#### Light IR Class

Class 定义同时继承 `Type` 和 `Value`.
以 int 为例，创建 int Class 时会生成两个类型和两个值，所以会同时继承 `Type` 和 `Value`.

```llvm
%$int$prototype_type = type {
  i32,
  i32,
  ptr,
  i32
}
@$int$prototype = global %$int$prototype_type {
  i32 1,
  i32 4,
  ptr @$int$dispatchTable,
  i32 0
}
%$int$dispatchTable_type = type {
  ptr
}
@$int$dispatchTable = global %$int$dispatchTable_type {
  ptr @$object.__init__
}
```

ChocoPy Class 在内存中的布局为：

1. Type tag, 四个字节，`*(int32_t*)addr`
2. Class 的大小是几个 word, `*((int32_t)addr + 4)`
3. Pointer to dispatch table, `*((int32_t)addr + 8)`
4. Attributes 1, `*((int32_t)addr + 12)`
5. Attributes 2, `*((int32_t)addr + 16)`
6. $\cdots$

以上描述和 ChocoPy Implementation Guide 4. Objects 的内容相同。

Type tag 用来区分不同的 Class，比如 `int` 和 `str` 的 Type tag 是不同的。
在 `print` 函数中，我们可以通过 Type tag 来判断一个 object 的类型，来执行正确的输出操作。
但是在自定义 Class 的 type tag 就没有什么作用了，因为我们没有运行时判断两个 class 是不是同一个类型的语法。
具体 type tag 定义见 ChocoPy Implementation Guide 4.1 Object layout.

`lightir::Class` 有 `add_attribute`, `add_method`, `get_offset_attr`, `get_attr_offset`, `get_method_offset` 等方法，可能会对你有所帮助。

### Light IR stdlib 调用

此部分参考 [src/cgen/stdlib/stdlib.c](../../src/cgen/stdlib/stdlib.c) 的代码，这部分代码比较简单，可直接阅读。
需要调用 stdlib 中的函数，只需要 `create_call` 即可，`LightWalker::LightWalker` 中已经声明了所有 stdlib 函数。

## 实验要求

### 代码结构

在 `include/ir-optimizer/chocopy_lightir.hpp` 中，定义了一个用于存储作用域的类 `ScopeAnalyzer`。
它的作用是辅助我们转换 `SymbolTable` 到 `Scope` ，管理不同作用域中的变量。它提供了以下接口：

```c++
// 进入一个新的作用域
void enter();
// 退出一个作用域
void exit();
// 往当前作用域插入新的名字->值映射
bool push(std::string name, Value *val);
// 根据名字，寻找到值
Value* find(std::string name);
// 根据名字，在 global 作用域寻找到值
Value* find_in_global(std::string name);
// 根据名字，在 nonlocal 作用域寻找到值，返回 load 好的值。
Value* find_in_nonlocal(std::string name, IRBuilder *builder);
// 判断当前是否在全局作用域内
bool in_global();
```

### 主要工作

修改 [chocopy_lightir.cpp](../../src/ir-optimizer/chocopy_lightir.cpp) 来实现生成 IR 的算法，使得它能正确编译任何合法的 ChocoPy 程序。

#### 几点说明

1. IR 生成部分没有做内存管理，我们也不会检查内存泄漏，只要程序正确得到 LLVM IR 即可。
2. 生成出来的 IR 也不用做内存管理，也就是说 ChocoPy 没有任何内存回收机制，不要求实现。
3. 提供的可能有 bug，虽然我们已经尽力去修了
   1. 代码框架中 IR 类型自动推导经常出错，可以使用 `Value::set_type` 来手动指定类型
   2. 如果你成功地修复或者部分修复了框架中的 bug，请及时联系我们。帮助我们持续改进这个项目。这将在 Code interview 部分中为你加分。


### 1.4 编译、运行和验证

* 编译

  若编译成功，则将在 `./[build_dir]/` 下生成 `ir-optimizer` 命令。

* 运行

  本次实验的 `ir-optimizer` 命令使用命令行参数来完成编译和运行。

  ```shell
  $ cd chocopy
  $ ./build/ir-optimizer test.py -run # 生成可执行文件并执行
  $ ./build/ir-optimizer test.py -emit # 输出 IR
  $ ./build/ir-optimizer test.py -assem # 用 llc 生成汇编
  ```

* 验证

  本次试验测试案例较多，为此我们将这些测试分为两类：

    1. sample: 这部分测试均比较简单且单纯，适合开发时调试。
    2. fuzz: 由 fuzzer 生成的 Python 文件。
    3. student: 这部分由同学提供。

  我们使用 `check.py` 进行评分。`check.py` 会将 `ir-optimizer` 的生成 IR 的运行结果和助教提供的 `*..py.typed.ll.result` 进行比较。

  ```shell
  cd chocopy/tests
  ./check.py --pa 3
  ```

  **请注意助教提供的 `testcase` 并不能涵盖全部的测试情况，完成此部分仅能拿到基础分，请自行设计自己的 `testcase` 进行测试。**

### 提供可用的测试用例

每组学生需要在 `tests/pa3/student/` 放置至少 4 个有意义的 `*.py` 测试案例，输出以 `.py.typed.ll.result` 结尾。

你的测试案例将被用来评估其他人的代码，你可以降低别人的分数。

### 评分

1. 基本测试样例[60pts]
2. Fuzzer 测试[5pts]
3. Student 测试[10pts]
4. 提供 Test Case[5pts]
5. Code interview[20pts]
