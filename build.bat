@echo off
echo Building RetroTerm...
g++ -std=c++17 -O2 -o retroterm.exe src/main.cpp src/game.cpp
echo Done: retroterm.exe
