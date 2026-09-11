#pragma once

#include <vector>
#include <string>
#include <cstdint>

enum class Tile : uint8_t {
    EMPTY  = ' ',
    WALL   = '#',
    FLOOR  = '.',
    DOOR   = '+',
    LOCKED = '%',
    PLAYER = '@',
    ITEM   = '*',
    STAIRS = '>',
    CORRIDOR = ',',
};

enum class EnemyType : uint8_t {
    GOBLIN,
    SKELETON,
    ORC,
    DRAGON,
    GHOST,
};

enum class ItemType : uint8_t {
    HEALTH_POTION,
    MANA_POTION,
    SWORD,
    SHIELD,
    KEY,
};

enum class SkillType : uint8_t {
    ATTACK,
    DEFEND,
    SPECIAL,
};

struct Point {
    int x, y;
    bool operator==(const Point& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Point& o) const { return !(*this == o); }
};

struct Entity {
    Point pos;
    int hp = 1;
    int max_hp = 1;
    int attack = 1;
    int defense = 0;
    std::string name;
    EnemyType type = EnemyType::GOBLIN;
    bool alive() const { return hp > 0; }
    int score_value = 10;
    int aggro_range = 6;
    bool enraged = false;
    bool can_phase = false;
};

struct Item {
    Point pos;
    std::string name;
    ItemType type = ItemType::HEALTH_POTION;
    int value = 0;
    bool collected = false;
};

struct InventorySlot {
    Item item;
    int count = 0;
};

struct SkillState {
    bool defending = false;
    int defend_turns = 0;
    int special_cooldown = 0;
    static constexpr int SPECIAL_MAX_COOLDOWN = 5;
    static constexpr int DEFEND_DURATION = 3;
    static constexpr int SPECIAL_MULTIPLIER = 3;
};

struct DungeonRoom {
    int x, y, w, h;
    int cx() const { return x + w / 2; }
    int cy() const { return y + h / 2; }
};

struct MinimapState {
    bool visible = false;
    std::vector<std::vector<bool>> explored;
    void init(int w, int h) {
        explored.assign(h, std::vector<bool>(w, false));
    }
    void reveal(const Point& p, int radius, int map_w, int map_h) {
        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                int nx = p.x + dx, ny = p.y + dy;
                if (nx >= 0 && nx < map_w && ny >= 0 && ny < map_h) {
                    explored[ny][nx] = true;
                }
            }
        }
    }
};

struct GameState {
    int width = 50;
    int height = 25;
    std::vector<std::vector<Tile>> map;
    Entity player;
    std::vector<Entity> enemies;
    std::vector<Item> items;
    std::vector<DungeonRoom> rooms;
    std::vector<InventorySlot> inventory;
    SkillState skills;
    MinimapState minimap;
    int level = 1;
    int max_level = 12;
    int turns = 0;
    bool running = true;
    std::string message;
    int score = 0;
    int enemies_killed = 0;
    int items_collected = 0;
    int floors_cleared = 0;
    int gold_collected = 0;
    bool dead = false;
};

void init_map(GameState& state);
void generate_level(GameState& state);
void render(const GameState& state);
void render_minimap(const GameState& state);
void handle_input(GameState& state, char input);
void handle_skill_input(GameState& state, char input);
void update_enemies(GameState& state);
bool check_collision(const GameState& state, Point pos);
bool line_of_sight(const GameState& state, Point from, Point to);
void place_entity(GameState& state, Entity& e);
void place_item(GameState& state, Item& item);
void game_loop(GameState& state);
void add_to_inventory(GameState& state, const Item& item);
void use_inventory_item(GameState& state, int index);
void open_inventory(GameState& state);

void death_screen(const GameState& state);
bool save_game(const GameState& state, const std::string& filename);
bool load_game(GameState& state, const std::string& filename);
Entity create_enemy(EnemyType type, int floor);
Item create_item(ItemType type);
ItemType random_item_type(int floor);
EnemyType random_enemy_type(int floor);
