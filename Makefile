CXX = c++
CXXFLAGS = -pthread -g3 -gdwarf-3 --std=c++03 -I$(GTEST_ROOT)/include

LD = c++
LDFLAGS = -g -pthread
LIBS = -L$(GTEST_ROOT)/lib -lgtest -lgtest_main

GTEST_ROOT=${HOME}/src/gtest-1.7.0

all: libmacro-test

OBJS = libmacro-test-obj-like.o libmacro-test-func-like.o libmacro.o

libmacro-test: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@ $(LIBS)
clean::
	$(RM) libmacro-test

%.o : %.cc
	$(CXX) -c $(CXXFLAGS) $<

clean::
	$(RM) $(OBJS) *~
