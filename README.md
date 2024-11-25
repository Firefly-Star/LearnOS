# 进栈不排队-OS原理赛道

## 参考文件

- [MIT-Xv6-Riscv源码](https://github.com/mit-pdos/xv6-riscv)
- [Xv6中文文档](https://th0ar.gitbooks.io/xv6-chinese/content/)
- [博客园：xv6-riscv内核调试教程](https://2017zhangyuxuan.github.io/2022/03/19/2022-03/2022-03-19%20%E7%8E%AF%E5%A2%83%E6%90%AD%E5%BB%BA%E7%B3%BB%E5%88%97-xv6%E5%86%85%E6%A0%B8%E8%B0%83%E8%AF%95%E6%95%99%E7%A8%8B/)
- [6.S081-All-In-One-Gitbook](https://xv6.dgs.zone/)

## 项目简介

本项目基于MIT开源的Xv6操作系统，使用RISC-V架构，实现了对操作系统内核的调试和修改。

### 项目结构

| 文件名 | 描述 |
| :---: | :---: |
| [mkfs](./mkfs/) | 构建和运行Xv6内核的Makefile |
| [README](./README) | 项目简介和说明 |
| [kernel](./kernel/) | 内核源代码目录 |
| [user](./user/) | 用户程序目录 |


### 主要功能

- 拓展了简单的系统调用

| 系统调用名 | 描述 |
| :---: | :---: |
| [getyear]() | 打印输出xv6内核创建时间 |

## 如何运行

### 运行环境

- Ubuntu 22.04 & Arch
- RISC-V 交叉编译器 QEMU模拟器

### qemu虚拟机运行

```bash
# ./xv6-riscv
make qemu
```

### 执行GDB调试

```bash
make qemu-gdb
```

## 贡献者

- [Linermao](https://github.com/Linermao)
- [wxz](https://github.com/Firefly-Star)