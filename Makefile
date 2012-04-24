CCACHE := $(shell which ccache 2>/dev/null)
CC = $(CCACHE) gcc
CXX = $(CCACHE) g++

CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lrt -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_calib3d

SRCFILES = $(wildcard src/*.cpp)
OBJFILES = $(patsubst src/%.cpp,bin/%.o,$(SRCFILES))
DEPFILES = $(patsubst src/%.cpp,dep/%.d,$(SRCFILES))

TARGET = ird

IDEPFILES = $(DEPFILES)
ifeq ($(MAKECMDGOALS),clean)
	IDEPFILES =
endif

.PHONY: all clean

all: $(TARGET)

clean:
	rm -f dep/* bin/* $(TARGET)

dep/%.d: src/%.cpp
	$(CXX) -MM -MT $(patsubst src/%.cpp,bin/%.o,$<) -MT $@ -MF $@ $<

bin/%.o: src/%.cpp
	$(CXX) $< -c -o $@ $(CFLAGS)

$(TARGET): $(OBJFILES)
	$(CXX) $^ -o $@ $(LDFLAGS)

-include $(IDEPFILES)
