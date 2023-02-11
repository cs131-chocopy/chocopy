FROM archlinux:latest
RUN echo "Server = https://mirrors.shanghaitech.edu.cn/archlinux/\$repo/os/\$arch" > /etc/pacman.d/mirrorlist && \
    pacman -Syu --noconfirm && \
    pacman -S --noconfirm fish neovim \
        git openssh \
        python python-termcolor python-natsort \
        gcc clang cmake make ninja fmt \
        flex bison \
        llvm riscv64-elf-binutils riscv64-elf-gcc riscv64-elf-newlib riscv64-elf-gdb \
        qemu-user
ENTRYPOINT ["/usr/bin/bash"]
