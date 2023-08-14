<p align="center">
  <a href="" rel="noopener">
 <img width=200px height=200px src="doc/figs/logo.jpg" alt="Project logo"></a>
</p>

<h3 align="center">Cat ARGB 无损压缩解压器</h3>


---

<p align="center"> 本项目为第七届集成电路创新创业大赛参赛作品 
<br> CICC2833 <b>复旦大学 /bin/cat 队</b>
<br> <b>景嘉微</b> 杯
</p>

## 📝 目录

- [算法简介](#about)
- [快速开始](#getting_started)
- [使用方法](#usage)
- [验证](#tests)
- [开发者信息](#authors)
- [致谢](#acknowledgement)

## 🧐 简介 <a name = "about"></a>

### 算法简介

我们使用的算法基于 LZ77，是一种**通用的**、**字典式**的算法，其主要优点如下所示：

- **硬件友好** 硬件实现难度低，适合于 FPGA 且快速、资源占用小
- **无损压缩** 压缩后的数据可以完全恢复
- **通用** 可以对任意长度的数据进行压缩
- **字节边界对齐** 便于与软件进行交互

### 硬件实现及仿真简介

我们使用 SpinalHDL 作为硬件实现语言，其简化了 HDL 的编写难度，且能产生规范化的 Verilog 文件，方便后续的仿真验证和在各种场合下的应用。

为了快速仿真验证，我们使用了 C++/Verilator 作为仿真工具。我们搭建了有效的仿真模型，在软件端虚拟化了硬件的行为，使得仿真速度大大提升。

## 🏁 快速开始 <a name = "getting_started"></a>

接下来的步骤将指导你如何进行 Verilog 文件的生成以及仿真验证。

### 必备工具

- [sbt](https://www.scala-sbt.org/)
- CMake
- Ninja
- [verilator](https://www.veripool.org/verilator/)
- GTKWave

### 软件验证

在项目根目录输入以下命令以构建：

```bash
mkdir build && cd build
cmake .. -G Ninja
ninja
```

你可以使用我们提供的脚本来进行整张图的软件验证。

回到项目根目录，输入以下命令以验证：

```bash
./check.sh res/sample06.bmp
```

你可以在`gen`目录下看到生成的两个文件，分别是压缩后的和经压缩后再解压的。

其余相关信息参见官方提供的 [README](doc/Official.md)。

我们实现的相关软件代码位于`src`中。

### 生成 Verilog

在项目根目录输入以下命令：

```bash
sbt run
```

成功后，会在 `rtl/verilog` 文件夹中看到相应的 Verilog 文件。

## 🔧 硬件仿真 <a name = "tests"></a>

在`build`目录输入以下命令以构建：

```bash
cmake .. -G Ninja -DSIM=ON
ninja
```

成功后，会在`build/sim/src`中看到`decode` `encode` `integrated` 三个文件夹，其中分别包含一个可执行文件，用来仿真解压器、压缩器以及统合的压缩解压器。

使用方法示例如下：

```bash
./sim/src/decode/sim_decode ../sim/examples/decode/sample01.txt 115
./sim/src/encode/sim_encode ../sim/examples/encode/sample02.txt 256
./sim/src/integrated/sim_core ../sim/examples/encode/sample02.txt 256
```

第一个参数用于指示**待解压/压缩文件的路径**，第二个参数用于指示**数据的字节数**。

命令执行成功后，会在当前路径下生成`.vcd`文件，用于在 GTKWave 中查看波形。

CLI也会产生类似如下的信息：

```
[INFO] Starting simulation...
[INFO] Waiting until done...
[INFO] Done.
[INFO] Encoded length: 62
[INFO] Returned to idle.
[INFO] Waiting until done...
[INFO] Done.
[INFO] Decoded length: 256
[INFO] Returned to idle.
[INFO] ========= Final result =========
[INFO] Encode cycles: 322
[INFO] Decode cycles: 108
[INFO] Total cycles: 435
[INFO] (^_^) Original memory and unencoded memory are equal.
```

## ✍️ 开发者信息 <a name = "authors"></a>

- [@0xtaruhi](https://github.com/0xtaruhi)
- [@ShaoqunLi](https://github.com/ShaoqunLi)
- [@Cheese](https://github.com/Meowcc)

## 🎉 致谢 <a name = "acknowledgement"></a>

以下是我们参考的资料：
- [Fast-LZ77](https://github.com/ariya/FastLZ)
