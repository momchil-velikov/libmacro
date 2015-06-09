# libmacro

`libmacro` is a tiny library implementation of the ISO C11 macro replacement specification. It is suitable for inclusion in IDEs/debuggers, which need to evaluate C/C++ expressions (e.g. in watch windows), based in the macro information in DWARF4 sections.

# Building

As simple as:

```
c++ -c --std=c++11 libmacro.cc
```

The library comes with testsuite and benchmark. For building these you need to install Google GTest (https://code.google.com/p/googletest) and Google Benchmark (https://github.com/google/benchmark). Edit the variables `GTEST_ROOT` and `BENCHMARK_ROOT` in the `Makefile` to point to the respective header/library locations.

# Usage

See the testsuite for examples.

# Limitations and bugs

No know bugs so far.

Only `C` locale is supported. C11 universal characters names, wide character and wide string literals are not implemented.

# Copyright and license

See file `LICENSE`
