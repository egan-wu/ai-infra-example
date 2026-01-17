CXX = g++
CXXFLAGS = -std=c++14 -I/usr/local/systemc/include -Wall -Wextra
LDFLAGS = -L/usr/local/systemc/lib -L/usr/local/systemc/lib-linux64 -lsystemc -lm

TARGET = simulation
SRC = main.cpp ram.cpp simplebus.cpp tpu.cpp cpu.cpp
OBJ = $(SRC:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJ)
