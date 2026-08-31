CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall
SRC = src/main.cpp src/game.cpp
OUT = retroterm

all: $(OUT)

$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(OUT)

.PHONY: all clean
