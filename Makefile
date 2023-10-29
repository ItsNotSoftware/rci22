# Project directories:
SRCDIR = src
INCDIR = headers
OBJDIR = obj

CXX = g++
CXXFLAGS = -Wall -g -Wextra -pedantic -std=c++11

# Source and object files
SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

# Targets
TARGET = cot
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ -I$(INCDIR)

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)
