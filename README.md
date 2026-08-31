# RetroTerm

Roguelike dungeon crawler with custom terminal rendering.

## Features

- Procedural dungeon generation
- Turn-based combat
- Enemy AI (chase player)
- Items: potions and gold
- Multiple levels with increasing difficulty
- ANSI color rendering
- Score tracking

## Build & Run

```bash
make
./retroterm

# or
g++ -std=c++17 -O2 -o retroterm src/main.cpp src/game.cpp
./retroterm
```

## Controls

- WASD — move
- > — descend stairs (when on stairs)
- Q — quit

## License

MIT
