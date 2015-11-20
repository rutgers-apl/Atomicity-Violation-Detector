ifneq (,$(shell which clang++ 2>/dev/null))
CXX=clang++
CXXFLAGS = -I$(TD_ROOT)/include -I$(TBBROOT)/include -DAV_ANALYSIS -ftaskdebug
else
CXX=g++
CXXFLAGS = -I$(TD_ROOT)/include -I$(TBBROOT)/include -DAV_ANALYSIS
endif # clang++

LIBS = -L$(TBBROOT)/obj -ltbb -L$(TD_ROOT)/obj -ltdebug

SRCS = $(wildcard *.cpp)

PROGS = $(patsubst %.cpp,%,$(SRCS))

all: $(PROGS)

%: %.cpp
	$(CXX) -g $(CXXFLAGS) -o $@ $< $(LIBS)

clean:
	rm -rf $(PROGS) *~ *.out util.pyc
