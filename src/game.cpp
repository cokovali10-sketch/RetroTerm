#include "game.h"
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <sstream>

void init_map(GameState& state) {
    state.map.resize(state.height, std::vector<Tile>(state.width, Tile::EMPTY));
}

void generate_level(GameState& state) {
    init_map(state);

    for (int y = 0; y < state.height; y++) {
        for (int x = 0; x < state.width; x++) {
            if (y == 0 || y == state.height - 1 || x == 0 || x == state.width - 1) {
                state.map[y][x] = Tile::WALL;
            } else {
                state.map[y][x] = Tile::FLOOR;
            }
        }
    }

    int num_walls = 20 + state.level * 5;
    for (int i = 0; i < num_walls; i++) {
        int x = 2 + rand() % (state.width - 4);
        int y = 2 + rand() % (state.height - 4);
        state.map[y][x] = Tile::WALL;
    }

    state.player.pos = {1, 1};
    state.player.hp = 10 + state.level * 2;
    state.player.attack = 2 + state.level;
    state.player.name = "Hero";
    state.map[1][1] = Tile::PLAYER;

    int num_enemies = 2 + state.level;
    state.enemies.clear();
    for (int i = 0; i < num_enemies; i++) {
        Entity e;
        e.name = "Goblin";
        e.hp = 3 + state.level;
        e.attack = 1 + state.level / 2;
        place_entity(state, e);
        state.enemies.push_back(e);
    }

    int num_items = 3 + state.level;
    state.items.clear();
    for (int i = 0; i < num_items; i++) {
        Item item;
        item.name = (rand() % 2 == 0) ? "Potion" : "Gold";
        item.value = (item.name == "Potion") ? 5 : 10 + rand() % 20;
        place_item(state, item);
        state.items.push_back(item);
    }

    int sx = state.width - 3;
    int sy = state.height - 3;
    state.map[sy][sx] = Tile::STAIRS;

    state.message = "Level " + std::to_string(state.level) + " — Find the stairs!";
}

void place_entity(GameState& state, Entity& e) {
    int attempts = 0;
    while (attempts < 1000) {
        int x = 1 + rand() % (state.width - 2);
        int y = 1 + rand() % (state.height - 2);
        if (state.map[y][x] == Tile::FLOOR) {
            e.pos = {x, y};
            state.map[y][x] = Tile::ENEMY;
            return;
        }
        attempts++;
    }
    e.pos = {state.width / 2, state.height / 2};
}

void place_item(GameState& state, Item& item) {
    int attempts = 0;
    while (attempts < 1000) {
        int x = 1 + rand() % (state.width - 2);
        int y = 1 + rand() % (state.height - 2);
        if (state.map[y][x] == Tile::FLOOR) {
            item.pos = {x, y};
            state.map[y][x] = Tile::ITEM;
            return;
        }
        attempts++;
    }
}

void render(const GameState& state) {
    std::cout << "\033[H\033[J";
    std::cout << "  ╔══════════════════════════════════════╗\n";
    std::cout << "  ║         R E T R O   T E R M          ║\n";
    std::cout << "  ╠══════════════════════════════════════╣\n";

    for (int y = 0; y < state.height; y++) {
        std::cout << "  ║ ";
        for (int x = 0; x < state.width; x++) {
            Tile t = state.map[y][x];
            switch (t) {
                case Tile::WALL:   std::cout << "\033[47m  \033[0m"; break;
                case Tile::FLOOR:  std::cout << ". "; break;
                case Tile::PLAYER: std::cout << "\033[1;32m@ \033[0m"; break;
                case Tile::ENEMY:  std::cout << "\033[1;31mE \033[0m"; break;
                case Tile::ITEM:   std::cout << "\033[1;33m* \033[0m"; break;
                case Tile::STAIRS: std::cout << "\033[1;36m> \033[0m"; break;
                default:           std::cout << "  "; break;
            }
        }
        std::cout << "║\n";
    }

    std::cout << "  ╠══════════════════════════════════════╣\n";
    std::cout << "  ║ HP: " << state.player.hp
              << "  ATK: " << state.player.attack
              << "  Score: " << state.score
              << "  Turn: " << state.turns << "\n";
    std::cout << "  ║ Level: " << state.level << "\n";
    if (!state.message.empty()) {
        std::cout << "  ║ " << state.message << "\n";
    }
    std::cout << "  ╚══════════════════════════════════════╝\n";
    std::cout << "  [WASD] Move  [Q] Quit  [>] Descend\n";
}

bool check_collision(const GameState& state, Point pos) {
    if (pos.x < 0 || pos.x >= state.width || pos.y < 0 || pos.y >= state.height)
        return true;
    return state.map[pos.y][pos.x] == Tile::WALL;
}

void handle_input(GameState& state, char input) {
    Point new_pos = state.player.pos;

    switch (input) {
        case 'w': case 'W': new_pos.y--; break;
        case 's': case 'S': new_pos.y++; break;
        case 'a': case 'A': new_pos.x--; break;
        case 'd': case 'D': new_pos.x++; break;
        case '>': {
            if (state.map[state.player.pos.y][state.player.pos.x] == Tile::STAIRS) {
                state.level++;
                generate_level(state);
            }
            return;
        }
        case 'q': case 'Q':
            state.running = false;
            return;
        default:
            return;
    }

    if (check_collision(state, new_pos)) {
        state.message = "You hit a wall!";
        return;
    }

    for (auto& enemy : state.enemies) {
        if (enemy.alive() && enemy.pos == new_pos) {
            enemy.hp -= state.player.attack;
            std::ostringstream oss;
            oss << "You hit " << enemy.name << " for " << state.player.attack << " damage!";
            state.message = oss.str();
            if (!enemy.alive()) {
                state.score += 10;
                state.map[new_pos.y][new_pos.x] = Tile::FLOOR;
                state.message += " Enemy defeated!";
            }
            state.turns++;
            return;
        }
    }

    for (auto& item : state.items) {
        if (!item.collected && item.pos == new_pos) {
            item.collected = true;
            state.map[new_pos.y][new_pos.x] = Tile::FLOOR;
            if (item.name == "Potion") {
                state.player.hp += item.value;
                state.message = "Healed for " + std::to_string(item.value) + " HP!";
            } else {
                state.score += item.value;
                state.message = "Found " + std::to_string(item.value) + " gold!";
            }
        }
    }

    state.map[state.player.pos.y][state.player.pos.x] = Tile::FLOOR;
    state.player.pos = new_pos;
    state.map[new_pos.y][new_pos.x] = Tile::PLAYER;
    state.turns++;
}

void update_enemies(GameState& state) {
    for (auto& enemy : state.enemies) {
        if (!enemy.alive()) continue;

        int dx = state.player.pos.x - enemy.pos.x;
        int dy = state.player.pos.y - enemy.pos.y;

        if (abs(dx) + abs(dy) == 1) {
            state.player.hp -= enemy.attack;
            state.message = enemy.name + " attacks for " + std::to_string(enemy.attack) + " damage!";
            if (state.player.hp <= 0) {
                state.message = "You died! Game Over.";
                state.running = false;
            }
            continue;
        }

        if (abs(dx) <= 3 && abs(dy) <= 3) {
            Point new_pos = enemy.pos;
            if (abs(dx) > abs(dy)) {
                new_pos.x += (dx > 0) ? 1 : -1;
            } else {
                new_pos.y += (dy > 0) ? 1 : -1;
            }
            if (!check_collision(state, new_pos) &&
                state.map[new_pos.y][new_pos.x] != Tile::PLAYER &&
                state.map[new_pos.y][new_pos.x] != Tile::ENEMY) {
                state.map[enemy.pos.y][enemy.pos.x] = Tile::FLOOR;
                enemy.pos = new_pos;
                state.map[new_pos.y][new_pos.x] = Tile::ENEMY;
            }
        }
    }
}

void game_loop(GameState& state) {
    std::srand(std::time(nullptr));
    generate_level(state);

    while (state.running) {
        render(state);
        char input;
        std::cin >> input;
        handle_input(state, input);
        if (state.running) {
            update_enemies(state);
        }
    }

    render(state);
    std::cout << "\n  Game Over! Final Score: " << state.score << "\n";
    std::cout << "  Levels completed: " << state.level - 1 << "\n";
    std::cout << "  Turns survived: " << state.turns << "\n\n";
}
