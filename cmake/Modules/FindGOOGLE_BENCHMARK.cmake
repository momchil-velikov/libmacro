find_path(GOOGLE_BENCHMARK_DIR
  NAMES
    include/benchmark/benchmark_api.h
  HINTS
    $ENV{HOME}/local/benchmark $ENV{HOME}/opt/benchmark /usr/local/benchmark
    /opt/benchmark
)
