#pragma once

#include <vector>
#include <string>

enum class Tile : char {
    EMPTY = ' ',
    WALL = '#',
    FLOOR = '.',
    DOOR = '+',
    PLAYER = '@',
    ENEMY = 'E',
    ITEM = '*',
    STAIRS = '>',
};

struct Point {
    int x, y;
    bool operator==(const Point& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Point& o) const { return !(*this == o); }
};

struct Entity {
    Point pos;
    int hp;
    int attack;
    std::string name;
    bool alive() const { return hp > 0; }
};

struct Item {
    Point pos;
    std::string name;
    int value;
    bool collected = false;
};

struct GameState {
    int width = 40;
    int height = 20;
    std::vector<std::vector<Tile>> map;
    Entity player;
    std::vector<Entity> enemies;
    std::vector<Item> items;
    int level = 1;
    int turns = 0;
    bool running = true;
    std::string message;
    int score = 0;
};

void init_map(GameState& state);
void generate_level(GameState& state);
void render(const GameState& state);
void handle_input(GameState& state, char input);
void update_enemies(GameState& state);
bool check_collision(const GameState& state, Point pos);
void place_entity(GameState& state, Entity& e);
void place_item(GameState& state, Item& item);
void game_loop(GameState& state);
