DEBUG=-g
PRODUCTION=-O3
CPP=g++-5
CFLAGS=-Wall -fmessage-length=0  -std=c++0x  -Wextra -pedantic -pedantic-errors $(PRODUCTION)
LDFLAGS=

# SOURCES.
SOURCES=GraphScheduler.cpp UDynGraph.cpp GraphSampler.cpp TriangleCounter.cpp Stats.cpp
BINARY_SOURCES=RunCounting.cpp RunCountingLocal.cpp 


# OBJECTS.
OBJECTS=$(SOURCES:.cpp=.o)
BINARY_OBJECTS=$(BINARY_SOURCES:.cpp=.o)
ALL_LOCAL_OBJECTS=$(OBJECTS) $(BINARY_OBJECTS)

# DEPENDENCIES.
DEPS=$(patsubst %.o,%.d,$(ALL_LOCAL_OBJECTS))

# BINARIES.
BINARIES=$(BINARY_SOURCES:.cpp=)

# RULES.

all: $(BINARIES) $(OBJECTS)

$(ALL_LOCAL_OBJECTS): %.o: %.cpp
	$(CPP) -MMD -MP $(CFLAGS) -c $< -o $@
	@sed -i -e '1s,\($*\)\.o[ :]*,\1.o $*.d: ,' $*.d

$(BINARIES): %: %.o $(OBJECTS)
	$(CPP) $^ $(LDFLAGS) -o $@

clean:
	rm -f $(DEPS) $(ALL_LOCAL_OBJECTS) $(BINARIES) *.d-e *.d

-include $(DEPS)
