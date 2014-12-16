CXX = c++
CXXFLAGS = -pthread -g3 -gdwarf-3 --std=c++03 -Wall -Wextra -I$(GTEST_ROOT)/include

LD = c++
LDFLAGS = -g -pthread
LIBS = -L$(GTEST_ROOT)/lib -lgtest -lgtest_main

GTEST_ROOT=${HOME}/src/gtest-1.7.0

all: libmacro-test

SRCS = libmacro.cc libmacro-test-obj-like.cc libmacro-test-func-like.cc
OBJS = $(SRCS:%.cc=%.o)
DEPS = $(SRCS:%.cc=%.d)

libmacro-test: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@ $(LIBS)
clean::
	$(RM) libmacro-test

%.o : %.cc
	$(CXX) -c $(CXXFLAGS) $< -o $@

%.d : %.cc
	$(CXX) -MM $(CXXFLAGS) $< -o $@

clean::
	$(RM) $(OBJS) $(DEPS) *~

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif
