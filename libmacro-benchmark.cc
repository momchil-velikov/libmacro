// -*- mode: c++; indent-tabs-mode: nil; -*-
#include "benchmark/benchmark.h"
#include "libmacro.hh"
#include <string>

namespace {

void
BM_macro_replacement(benchmark::State& state) {
  libmacro::macro_table macros;
  macros.add_define(1, "A() D(u,E(u,v))");
  macros.add_define(2, "B(x) E(x,F(x,v,w))");
  macros.add_define(3, "C(x) F(x,E(x,v),w)");
  macros.add_define(4, "D(x,y) F(x,E(x,y),w)");
  macros.add_define(5, "E(x,y) F(x,y,w).");
  macros.add_define(6, "F(x,y,z) D(F(x,y,z),E(z,x))");

  while (state.KeepRunning())
    libmacro::macro_expand("B(a) C(a) D(e,f) E(f,g) F(g,h,i)"
                           "B(a) C(a) D(e,f) E(f,g) F(g,h,i)"
                           "B(a) C(a) D(e,f) E(f,g) F(g,h,i)"
                           "B(a) C(a) D(e,f) E(f,g) F(g,h,i)",
                           &macros, 0);
}

}

BENCHMARK(BM_macro_replacement);

int main(int argc, const char** argv) {
  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();

  return 0;
}
