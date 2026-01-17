CXX = g++
CXXFLAGS = -std=c++14 -I/usr/local/systemc/include -Wall -Wextra
LDFLAGS = -L/usr/local/systemc/lib -L/usr/local/systemc/lib-linux64 -lsystemc -lm

TARGET = simulation
SRC = main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
