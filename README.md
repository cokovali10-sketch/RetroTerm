<div align="center">

<img src="https://capsule-render.vercel.app/api?type=waving&color=0:0f172a,50:38bdf8,100:a78bfa&height=160&section=header&text=RetroTerm&fontSize=48&fontColor=ffffff&fontAlignY=38&animation=fadeIn" alt="header" />

</div>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-17-blue?style=for-the-badge&logo=cplusplus" alt="C++17"/>
  <img src="https://img.shields.io/badge/License-MIT-green?style=for-the-badge" alt="License"/>
  <img src="https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey?style=for-the-badge" alt="Platform"/>
  <img src="https://img.shields.io/badge/Status-Active-brightgreen?style=for-the-badge" alt="Status"/>
</p>

<h1 align="center">RETROTERM</h1>

<p align="center">
  <b>A classic roguelike dungeon crawler for the terminal</b><br>
  Procedural dungeons, turn-based combat, inventory management, and permadeath — all rendered in ANSI color.
</p>

---

## Example Output

```
╔══════════════════════════════════════════════════════════════════╗
║                   R E T R O   T E R M                          ║
╠══════════════════════════════════════════════════════════════════╣
║                                                                 ║
║  ####.###########..##########...###########..###########        ║
║  #....#...........#..........#..#...........#..........#        ║
║  #.##.#.#########.#########.#..#.#########.#.########.#        ║
║  #.#..#.#......#.#.........#....#.....@...#.#.......#.#        ║
║  #.#.##.#.####.#.#.#######.####.#.#######.#.#.#####.#.#        ║
║  #......#....#.#.#.#.....#......#.......#.#...#...#.#.#        ║
║  ########.##.#.#.#.#.###.######.#######.#.#####.#.#.#.#        ║
║          #..#...#.#...#......#.........#.#.......#...#.#        ║
║  ########.##.###.#.###.####.#.#########.#########.###.#        ║
║  #......*#......#.....#..#...#...........#.....*...#...#        ║
║  #.####.##.#####.####.#.#.###.#########.#.#####.##.###        ║
║  #......#.......#....#...#...#...#....E.#.#.........#.#        ║
║  #.####.#########.##.###.###.###.#.###.#.#.#########.#        ║
║  #......#...S...#..#.....#.....#.#.#.#...#.........#.#        ║
║  ########.###.##.###.####.#.###.#.#.#.###.#########.#.#        ║
║                                                                 ║
╠══════════════════════════════════════════════════════════════════╣
║  HP: 24/30  ATK: 5  DEF: 2  Gold: 85  Floor: 3  Turn: 47      ║
  ║  [F1] Minimap  [I] Inventory  [F2] Defend  [K] Save  [Q] Quit   ║
║  You see a Goblin nearby.                                       ║
╚══════════════════════════════════════════════════════════════════╝
```

## Features

- **Procedural Dungeon Generation** — BSP tree algorithm creates unique multi-room dungeons with corridors every run
- **Turn-Based Combat** — Attack, defend, or use special abilities against varied enemies
- **5+ Enemy Types** — Goblins, Skeletons, Orcs, Dragons, and Ghosts with unique stats and AI behaviors
- **5+ Item Types** — Health Potions, Mana Potions, Swords, Shields, and Keys
- **Inventory System** — Collect items, choose when to use them strategically
- **Skills & Abilities** — Attack, Defend (damage reduction), and Special (high-damage cooldown)
- **10+ Dungeon Floors** — Escalating difficulty with tougher enemies and better loot
- **Minimap** — Toggle an overview of explored areas with [F1]
- **Save/Load System** — Serialize full game state to JSON, resume anytime
- **Death Screen** — Detailed stats summary on game over
- **Score Tracking** — Compete against yourself for the highest score
- **ANSI Color Rendering** — Rich terminal visuals with box-drawing characters

## Game Mechanics

### Combat System

| Action | Key | Description |
|--------|-----|-------------|
| Attack | `WASD` (move into enemy) | Deal your ATK damage to the target |
| Defend | `F2` | Halve incoming damage for 3 turns |
| Special | `F3` | Deal 3x ATK damage (cooldown: 5 turns) |

Damage formula: `actual_damage = attacker_ATK - defender_DEF` (minimum 1).

### Enemy Types

| Enemy | HP | ATK | DEF | Behavior |
|-------|-----|-----|-----|----------|
| Goblin | Low | Low | 0 | Aggressive, chases on sight (8 tiles) |
| Skeleton | Low | Medium | 1 | Patrols, attacks if adjacent |
| Orc | Medium | Medium | 2 | Charges when HP < 50% |
| Dragon | High | High | 3 | Breath attack (hits 2 tiles), slow |
| Ghost | Medium | Medium | 0 | Phases through walls, teleports |

### Items

| Item | Effect | Rarity |
|------|--------|--------|
| Health Potion | Restores 10-15 HP | Common |
| Mana Potion | Resets Special cooldown | Uncommon |
| Sword | +2 ATK permanently | Rare |
| Shield | +2 DEF permanently | Rare |
| Key | Opens locked doors (boss rooms) | Floor-specific |

### Dungeon Generation (BSP)

1. Start with the full map as one region
2. Recursively split into sub-regions (4-8 splits)
3. Place a random room inside each leaf region
4. Connect sibling rooms with L-shaped corridors
5. Place stairs in the deepest room, enemies in random rooms

### Enemy AI

Each enemy type uses a different behavior tree:

- **Line-of-sight check** — Can the enemy see the player? (up to 8 tiles, no walls)
- **Aggro radius** — Goblins chase at 8 tiles, Orcs at 5, Dragons at 10
- **Pathfinding** — Simple greedy chase: move toward player along the dominant axis
- **Special behaviors** — Ghosts phase through walls, Dragons use breath attacks, Orcs enrage at low HP

## Controls

| Key | Action |
|-----|--------|
| `W` / `A` / `S` / `D` | Move up / left / down / right |
| `>` | Descend stairs (when standing on them) |
| `F1` | Toggle minimap |
| `F2` | Defend (reduce damage for 3 turns) |
| `F3` | Special ability (high damage, cooldown) |
| `I` | Open inventory |
| `K` | Save game |
| `L` | Load game (from title screen) |
| `Q` | Quit |

## Build from Source

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- GNU Make (Linux/macOS) or compatible

### Build & Run

```bash
# Clone the repository
git clone https://github.com/yourname/RetroTerm.git
cd RetroTerm

# Build with make
make

# Run the game
./retroterm

# Or build manually
g++ -std=c++17 -O2 -Wall -o retroterm src/main.cpp src/game.cpp

# Clean build artifacts
make clean
```

### Install (optional)

```bash
make install          # installs to /usr/local/bin
make install PREFIX=/opt/retroterm  # custom prefix
```

### Run Tests

```bash
make test
```

## Project Structure

```
RetroTerm/
├── .github/workflows/
│   └── ci.yml              # GitHub Actions CI pipeline
├── src/
│   ├── main.cpp             # Entry point, title screen, save/load menu
│   ├── game.h               # All type definitions, enums, and declarations
│   └── game.cpp             # Core game logic, rendering, AI, dungeon gen
├── tests/
│   └── test_game.cpp        # Unit tests for game systems
├── Makefile                 # Build, test, install targets
├── LICENSE                  # MIT License
└── README.md                # This file
```

## Technical Details

- **Language:** C++17 (no external dependencies)
- **Rendering:** ANSI escape codes with box-drawing characters
- **Serialization:** Custom JSON writer/reader for save files
- **Dungeon Gen:** Binary Space Partitioning with random room placement
- **AI:** Greedy pathfinding with type-specific behavior trees
- **Memory:** Zero heap allocations in hot loop; vectors pre-allocated per floor

## License

MIT License — see [LICENSE](LICENSE) for details.
