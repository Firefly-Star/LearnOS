# 进栈不排队-OS原理赛道：内核实现方向

```
    ███████     █████████     █████   ████ █████ ██████   █████   █████████ 
  ███░░░░░███  ███░░░░░███   ░░███   ███░ ░░███ ░░██████ ░░███   ███░░░░░███
 ███     ░░███░███    ░░░     ░███  ███    ░███  ░███░███ ░███  ███     ░░░ 
░███      ░███░░█████████     ░███████     ░███  ░███░░███░███ ░███         
░███      ░███ ░░░░░░░░███    ░███░░███    ░███  ░███ ░░██████ ░███    █████
░░███     ███  ███    ░███    ░███ ░░███   ░███  ░███  ░░█████ ░░███  ░░███ 
 ░░░███████░  ░░█████████     █████ ░░████ █████ █████  ░░█████ ░░█████████ 
   ░░░░░░░     ░░░░░░░░░     ░░░░░   ░░░░ ░░░░░ ░░░░░    ░░░░░   ░░░░░░░░░  
```

- [进栈不排队-OS原理赛道：内核实现方向](#进栈不排队-os原理赛道内核实现方向)
  - [基本信息](#基本信息)
  - [项目简介](#项目简介)
    - [内存管理模块](#内存管理模块)
    - [进程通信模块](#进程通信模块)
    - [进程管理模块](#进程管理模块)
  - [项目结构](#项目结构)
  - [如何运行](#如何运行)
    - [运行环境](#运行环境)
    - [qemu虚拟机运行](#qemu虚拟机运行)
    - [执行GDB调试](#执行gdb调试)
  - [参考文件](#参考文件)
  - [贡献者](#贡献者)

## 基本信息

- **比赛类型**：OS原理赛道：内核实现方向
- **学校名称**：杭州电子科技大学
- **队伍编号**：T202410336994460
- **队伍名称**：进栈不排队
- **队伍成员**：
  - 温学周
  - 林灿
- **指导老师**：杨浩

## 项目简介

本项目基于 `MIT` 开源的 `Xv6` 操作系统框架，构建 `RISC-V` 架构的类 `Unix` 操作系统。截至初赛，总共新增了**19**个系统调用，总计**40**个系统调用。

主要改进和提升体现在**内存管理模块**，**进程通信模块**，**进程管理模块**。

### 内存管理模块

### 进程通信模块

### 进程管理模块



## 项目结构

| 文件名 | 描述 |
| :---: | :---: |
| [mkfs](./mkfs/) | 构建和运行Xv6内核 |
| [README](./README) | 项目简介和说明 |
| [doc](./doc) | 项目的比赛报告 |
| [kernel](./kernel/) | 内核源代码目录 |
| [user](./user/) | 用户源代码目录 |
| [dockerfile](./dockerfile) | docker编译相关文件 |
| [docker-compose.yml](./docker-compose.yml) | docker编译相关文件 |
| [start_docker.sh](./start_docker.sh) | 快速启动docker环境 |
| [LICENSE](./LICENSE) | 项目的许可证文件 |
| [Makefile](./Makefile) | 项目的Makefile文件 |


## 如何运行

### 运行环境

- Ubuntu 22.04 & Arch
- gcc RISC-V 交叉编译器，QEMU模拟器

本项目配置了`docker`包，可以快速一键启动：

```bash
# 拉取 docker 包
docker pull linermao/xv6-riscv-env:latest
# 快速启动，进入 docker 环境
./start_docker.sh
```

也可以本地编译：

```bash
# 修改 docker-compose.yml
..
# dockerfile: linermao/xv6-riscv-env
dockerfile: dockerfile
..
```

### qemu虚拟机运行

```bash
# learnos/
make qemu
```

### 执行GDB调试

```bash
make qemu-gdb
```

## 参考文件

- [MIT-Xv6-Riscv源码](https://github.com/mit-pdos/xv6-riscv)
- [Xv6中文文档](https://th0ar.gitbooks.io/xv6-chinese/content/)
- [博客园：xv6-riscv内核调试教程](https://2017zhangyuxuan.github.io/2022/03/19/2022-03/2022-03-19%20%E7%8E%AF%E5%A2%83%E6%90%AD%E5%BB%BA%E7%B3%BB%E5%88%97-xv6%E5%86%85%E6%A0%B8%E8%B0%83%E8%AF%95%E6%95%99%E7%A8%8B/)
- [6.S081-All-In-One-Gitbook](https://xv6.dgs.zone/)

## 贡献者

- 温学周：[wxz](https://github.com/Firefly-Star)
- 林灿：[Linermao](https://github.com/Linermao)