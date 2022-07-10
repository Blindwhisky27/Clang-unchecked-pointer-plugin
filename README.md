# Clang-unchecked-pointer-plugin
A clang plugin to check if a C code uses unchecked pointer after allocating memory using malloc().

# Requirements

```
Clang-11
Cmake
```

# Instructions
To build

```
cmake build .
cmake --build .
```

To compile and run plugin

```
cmake --build && clang -fplugin=./CheckMalloc.so filename.c
```

# Credits
Original repo: https://github.com/mahesh-hegde/clang_diag_plugin

