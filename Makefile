CXX = g++
CXXFLAGS_COMMON = -std=c++14 -I/usr/local/systemc/include -Wall -Wextra
LDFLAGS_COMMON = -L/usr/local/systemc/lib -L/usr/local/systemc/lib-linux64 -lsystemc -lm

# Simple Mode
TARGET_SIMPLE = simulation
SRC_SIMPLE = main.cpp ram.cpp simplebus.cpp tpu.cpp cpu.cpp
OBJ_SIMPLE = $(SRC_SIMPLE:.cpp=.o)

# Gem5 Mode
TARGET_GEM5 = simulation_gem5
SRC_GEM5 = main.cpp ram.cpp simplebus.cpp tpu.cpp gem5_wrapper.cpp
# Note: we don't include cpu.cpp here usually, or if we do, it's unused.
# We need to distinguish main.o for simple vs gem5.

# Gem5 Configuration
GEM5_HOME = /opt/gem5
PYTHON_FLAGS = $(shell python3-config --ldflags --embed 2>/dev/null || python3-config --ldflags)
CXXFLAGS_GEM5 = $(CXXFLAGS_COMMON) -DUSE_GEM5 -I$(GEM5_HOME)/build/ARM -I$(GEM5_HOME)/src
LDFLAGS_GEM5 = $(LDFLAGS_COMMON) -L$(GEM5_HOME)/build/ARM -lgem5_opt -lprotobuf $(PYTHON_FLAGS)

all: simple

# --- Simple Target ---
simple: $(TARGET_SIMPLE)

$(TARGET_SIMPLE): $(OBJ_SIMPLE)
	$(CXX) $(CXXFLAGS_COMMON) -o $(TARGET_SIMPLE) $(OBJ_SIMPLE) $(LDFLAGS_COMMON)

# --- Gem5 Target ---
gem5: $(TARGET_GEM5)

# We define specific objects for Gem5 build to avoid collision
OBJ_GEM5_ALONE = gem5_wrapper.o main_gem5.o
OBJ_SHARED = ram.o simplebus.o tpu.o

$(TARGET_GEM5): $(OBJ_GEM5_ALONE) $(OBJ_SHARED)
	$(CXX) $(CXXFLAGS_GEM5) -o $(TARGET_GEM5) $(OBJ_GEM5_ALONE) $(OBJ_SHARED) $(LDFLAGS_GEM5)

# --- Object Rules ---

# Standard objects (Simple Mode)
%.o: %.cpp
	$(CXX) $(CXXFLAGS_COMMON) -c $< -o $@

# Gem5 objects
main_gem5.o: main.cpp
	$(CXX) $(CXXFLAGS_GEM5) -c main.cpp -o main_gem5.o

gem5_wrapper.o: gem5_wrapper.cpp
	$(CXX) $(CXXFLAGS_GEM5) -c gem5_wrapper.cpp -o gem5_wrapper.o

# Run Rules
run_simple: simple
	./$(TARGET_SIMPLE)

run_gem5: gem5
	./$(TARGET_GEM5)

clean:
	rm -f $(TARGET_SIMPLE) $(TARGET_GEM5) *.o
