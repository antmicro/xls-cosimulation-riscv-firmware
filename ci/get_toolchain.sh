#!/bin/sh

wget -O toolchain.tar.gz https://github.com/chipsalliance/caliptra-tools/releases/download/gcc-v12.1.0/riscv64-unknown-elf.gcc-12.1.0.tar.gz
tar -xzvf toolchain.tar.gz
export TOOLCHAIN_PREFIX=`pwd`/riscv/bin/riscv64-unknown-elf-
