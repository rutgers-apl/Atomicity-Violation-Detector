ifneq (,$(shell which clang++ 2>/dev/null))
CXX=clang++
else
CXX=g++
endif # clang++

CXXFLAGS+= -I$(TD_ROOT)/include -I$(TBBROOT)/include -DAV_ANALYSIS -std=c++11
LIBS+= -L$(TBBROOT)/obj -ltbb -L$(TD_ROOT)/obj -ltdebug

SRCS = trace_executor.cpp

PROGS = $(patsubst %.cpp,%,$(SRCS))

all: $(PROGS)

%: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LIBS)

clean:
	rm -rf $(PROGS) *~ *.out util.pyc trace.txt
