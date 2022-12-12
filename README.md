# lc3-toolchain
LC-3 toolchain implemented in c.

## Purpose
Implementing an LC-3 toolchain in C for fun. Ultimate goal is to enable
developing directly on an LC-3 device. First step is to build a toolchain which
allows developing for LC-3 on existing platforms to eventually bootstrap on an
LC-3 device.

## Usage
Built using GNU Make and GNU GCC. Main artifact is `out/lc3asm`.

Main targets are:
- `all`: builds main artifact `out/lc3asm`.
- `clean`: clears `out` directory and removes all precompiled headers from `src`.
- `hello`: depends on `all`, but also builds `out/hello.obj` from `hello.asm`, and shows `out/hello.obj` using `hexdump -C`.

