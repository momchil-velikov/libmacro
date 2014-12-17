ifeq ($(R),1)
OPTFLAGS = -Ofast
else
OPTFLAGS = -O0 -g3 -gdwarf-3
endif

CXX = c++
CXXFLAGS = $(OPTFLAGS) -pthread --std=c++11 -Wall -Wextra -I$(GTEST_ROOT)/include -I$(BENCHMARK_ROOT)/include

LD = $(CXX)
LDFLAGS = -g -pthread
LIBS = -L$(GTEST_ROOT)/lib -lgtest -lgtest_main -L$(BENCHMARK_ROOT)/src -lbenchmark


GTEST_ROOT=${HOME}/src/gtest-1.7.0
BENCHMARK_ROOT=${HOME}/src/benchmark

all: libmacro-test libmacro-benchmark


LIBSRCS = libmacro.cc
LIBOBJS = $(LIBSRCS:%.cc=%.o)

TESTSRCS = libmacro-test-obj-like.cc libmacro-test-func-like.cc
TESTOBJS = $(TESTSRCS:%.cc=%.o)

BENCHSRCS = libmacro-benchmark.cc
BENCHOBJS = $(BENCHSRCS:%.cc=%.o)

SRCS = $(LIBSRCS) $(TESTSRCS) $(BENCHSRCS)
OBJS = $(SRCS:%.cc=%.o)
DEPS = $(SRCS:%.cc=%.d)

libmacro-test: $(LIBOBJS) $(TESTOBJS)
	$(LD) $(LDFLAGS) $(LIBOBJS) $(TESTOBJS) -o $@ $(LIBS)
clean::
	$(RM) libmacro-test

libmacro-benchmark: $(LIBOBJS) $(BENCHOBJS)
	$(LD) $(LDFLAGS) $(LIBOBJS) $(BENCHOBJS) -o $@ $(LIBS)
clean::
	$(RM) libmacro-benchmark

%.o : %.cc
	$(CXX) -c $(CXXFLAGS) $< -o $@

%.d : %.cc
	$(CXX) -MM $(CXXFLAGS) $< -o $@

clean::
	$(RM) $(OBJS) $(DEPS) *~

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif
