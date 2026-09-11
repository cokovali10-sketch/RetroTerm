CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Wpedantic
SRC      = src/main.cpp src/game.cpp
OUT      = retroterm
PREFIX   ?= /usr/local

# ── Default ───────────────────────────────────────────────────────────────
all: $(OUT)

$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

# ── Clean ─────────────────────────────────────────────────────────────────
clean:
	rm -f $(OUT) savegame.json
	rm -rf tests/*.o tests/test_game tests/test_game.exe

# ── Install ───────────────────────────────────────────────────────────────
install: $(OUT)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(OUT) $(DESTDIR)$(PREFIX)/bin/$(OUT)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(OUT)

# ── Test ──────────────────────────────────────────────────────────────────
test: tests/test_game
	./tests/test_game

tests/test_game: tests/test_game.cpp src/game.cpp src/game.h
	$(CXX) $(CXXFLAGS) -Isrc -o $@ tests/test_game.cpp src/game.cpp

.PHONY: all clean install uninstall test
