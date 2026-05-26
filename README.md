# LearnOS — Operating System Learning Project

An operating system learning project based on Xv6 (RISC-V), exploring OS kernel principles through hands-on experiments.

## Description

This project is part of the "Stack-In OS" competition track. It involves modifying and extending the Xv6 RISC-V operating system with custom system calls, scheduling algorithms, and kernel modules.

## Features

- Xv6 RISC-V kernel modifications
- Custom system call implementation
- Process scheduling experiments
- GDB debugging support
- QEMU virtualization

## Quick Start

### Prerequisites

- RISC-V toolchain
- QEMU (RISC-V version)
- GDB (for debugging)

### Run with QEMU

```bash
make qemu
```

### Debug with GDB

```bash
make qemu-gdb
# In another terminal
gdb-multiarch
```

## Project Structure

```
kernel/     — Kernel source code
user/       — User-space programs
doc/        — Documentation and demo videos
```
