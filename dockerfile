# 基础镜像
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# 设置环境变量
ENV DEBIAN_FRONTEND=noninteractive
ENV RISCV=/opt/riscv
ENV PATH=$RISCV/bin:$PATH

# 更新系统并安装依赖
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    gcc \
    g++ \
    git \
    curl \
    bison \
    wget \
    flex \
    ninja-build \
    pkg-config \
    libgmp-dev \
    libmpc-dev \
    libmpfr-dev \
    libglib2.0-dev \
    zlib1g-dev \
    libpixman-1-dev \
    libtool \
    autoconf \
    gettext \
    libcap-ng-dev \
    libfdt-dev \
    libgcrypt20-dev \
    libseccomp-dev \
    gcc-riscv64-unknown-elf \
    gcc-riscv64-linux-gnu \
    binutils-riscv64-linux-gnu \
    texinfo \
    python3 \
    python3-pip \
    gdb-multiarch \
    qemu \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# 版本问题，太高或太低都会导致出错
RUN wget https://download.qemu.org/qemu-7.2.0.tar.xz && \
    tar -xvf qemu-7.2.0.tar.xz && \
    cd qemu-7.2.0 && \
    ./configure --target-list=riscv64-softmmu,riscv64-linux-user && \
    make -j$(nproc) && \
    make install && \
    cd .. && \
    rm -rf qemu-7.2.0 qemu-7.2.0.tar.xz

# 设置工作目录
WORKDIR /learnos

# 挂载卷（仅在容器运行时可用）
VOLUME ["/learnos"]

# 默认启动命令
CMD ["bash"]
