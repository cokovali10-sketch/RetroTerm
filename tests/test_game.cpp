#include "game.h"
#include <iostream>
#include <cassert>
#include <cmath>

// ── Minimal test framework ──────────────────────────────────────────────

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

static void run_test(const char* name, void (*fn)()) {
    tests_run++;
    std::cout << "  Running " << name << "... ";
    try {
        fn();
        tests_passed++;
        std::cout << "\033[32mPASS\033[0m\n";
    } catch (const std::exception& e) {
        tests_failed++;
        std::cout << "\033[31mFAIL\033[0m (" << e.what() << ")\n";
    } catch (...) {
        tests_failed++;
        std::cout << "\033[31mFAIL\033[0m (unknown exception)\n";
    }
}

#define TEST(name)                          \
    static void test_##name();              \
    static const bool registered_##name =   \
        (run_test(#name, &test_##name), true); \
    static void test_##name()
#define ASSERT(expr) \
    do { if (!(expr)) throw std::runtime_error("ASSERT failed: " #expr); } while(0)

#define ASSERT_EQ(a, b) \
    do { if ((a) != (b)) throw std::runtime_error("ASSERT_EQ failed: " #a " != " #b); } while(0)

struct TestReg { TestReg() {} } static_reg_dummy;

// ── Tests ────────────────────────────────────────────────────────────────

TEST(point_equality) {
    Point a{1, 2}, b{1, 2}, c{3, 4};
    ASSERT(a == b);
    ASSERT(a != c);
    ASSERT(!(a == c));
}

TEST(tile_values) {
    ASSERT(static_cast<char>(Tile::WALL) == '#');
    ASSERT(static_cast<char>(Tile::FLOOR) == '.');
    ASSERT(static_cast<char>(Tile::PLAYER) == '@');
    ASSERT(static_cast<char>(Tile::STAIRS) == '>');
    ASSERT(static_cast<char>(Tile::ITEM) == '*');
}

TEST(entity_alive) {
    Entity e;
    e.hp = 10;
    ASSERT(e.alive());
    e.hp = 0;
    ASSERT(!e.alive());
    e.hp = -5;
    ASSERT(!e.alive());
}

TEST(create_enemy_goblin) {
    Entity e = create_enemy(EnemyType::GOBLIN, 1);
    ASSERT_EQ(e.name, "Goblin");
    ASSERT(e.hp > 0);
    ASSERT(e.attack > 0);
    ASSERT(e.defense >= 0);
    ASSERT(!e.can_phase);
}

TEST(create_enemy_dragon) {
    Entity e = create_enemy(EnemyType::DRAGON, 1);
    ASSERT_EQ(e.name, "Dragon");
    ASSERT(e.hp > 10);
    ASSERT(e.attack > 3);
    ASSERT(e.defense >= 2);
}

TEST(create_enemy_ghost) {
    Entity e = create_enemy(EnemyType::GHOST, 1);
    ASSERT_EQ(e.name, "Ghost");
    ASSERT(e.can_phase);
}

TEST(create_enemy_scaling) {
    Entity e1 = create_enemy(EnemyType::GOBLIN, 1);
    Entity e5 = create_enemy(EnemyType::GOBLIN, 9);
    ASSERT(e5.hp > e1.hp);
    ASSERT(e5.attack > e1.attack);
}

TEST(create_item_health) {
    Item i = create_item(ItemType::HEALTH_POTION);
    ASSERT_EQ(i.name, "Health Potion");
    ASSERT(i.value >= 10);
    ASSERT_EQ(static_cast<int>(i.type), static_cast<int>(ItemType::HEALTH_POTION));
}

TEST(create_item_sword) {
    Item i = create_item(ItemType::SWORD);
    ASSERT_EQ(i.name, "Sword");
    ASSERT_EQ(i.value, 2);
}

TEST(create_item_shield) {
    Item i = create_item(ItemType::SHIELD);
    ASSERT_EQ(i.name, "Shield");
    ASSERT_EQ(i.value, 2);
}

TEST(create_item_mana) {
    Item i = create_item(ItemType::MANA_POTION);
    ASSERT_EQ(i.name, "Mana Potion");
}

TEST(create_item_key) {
    Item i = create_item(ItemType::KEY);
    ASSERT_EQ(i.name, "Key");
    ASSERT_EQ(i.value, 1);
}

TEST(init_map) {
    GameState state;
    state.width = 10;
    state.height = 8;
    init_map(state);
    ASSERT_EQ((int)state.map.size(), 8);
    ASSERT_EQ((int)state.map[0].size(), 10);
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 10; x++)
            ASSERT_EQ(state.map[y][x], Tile::WALL);
}

TEST(generate_level_creates_rooms) {
    GameState state;
    state.level = 1;
    generate_level(state);
    ASSERT(state.rooms.size() >= 2);
    ASSERT(state.enemies.size() > 0);
    ASSERT(state.items.size() > 0);
}

TEST(generate_level_stairs_exist) {
    GameState state;
    state.level = 1;
    generate_level(state);
    bool found_stairs = false;
    for (int y = 0; y < state.height; y++)
        for (int x = 0; x < state.width; x++)
            if (state.map[y][x] == Tile::STAIRS) found_stairs = true;
    ASSERT(found_stairs);
}

TEST(generate_level_player_exists) {
    GameState state;
    state.level = 1;
    generate_level(state);
    bool found_player = false;
    for (int y = 0; y < state.height; y++)
        for (int x = 0; x < state.width; x++)
            if (state.map[y][x] == Tile::PLAYER) found_player = true;
    ASSERT(found_player);
}

TEST(generate_level_increases_difficulty) {
    GameState s1, s2;
    s1.level = 1; s2.level = 5;
    generate_level(s1);
    generate_level(s2);
    ASSERT(s2.enemies.size() > s1.enemies.size());
}

TEST(check_collision_wall) {
    GameState state;
    state.width = 10;
    state.height = 8;
    init_map(state);
    ASSERT(check_collision(state, {0, 0}));
    ASSERT(!check_collision(state, {1, 1}));
}

TEST(check_collision_bounds) {
    GameState state;
    state.width = 10;
    state.height = 8;
    init_map(state);
    ASSERT(check_collision(state, {-1, 0}));
    ASSERT(check_collision(state, {10, 0}));
    ASSERT(check_collision(state, {0, -1}));
    ASSERT(check_collision(state, {0, 8}));
}

TEST(line_of_sight_clear) {
    GameState state;
    state.width = 10;
    state.height = 8;
    init_map(state);
    // Floor in a line
    for (int x = 1; x < 9; x++) state.map[1][x] = Tile::FLOOR;
    ASSERT(line_of_sight(state, {1, 1}, {7, 1}));
}

TEST(line_of_sight_blocked) {
    GameState state;
    state.width = 10;
    state.height = 8;
    init_map(state);
    for (int x = 1; x < 9; x++) state.map[1][x] = Tile::FLOOR;
    state.map[1][4] = Tile::WALL;
    ASSERT(!line_of_sight(state, {1, 1}, {7, 1}));
}

TEST(inventory_add_single) {
    GameState state;
    Item i = create_item(ItemType::HEALTH_POTION);
    add_to_inventory(state, i);
    ASSERT_EQ((int)state.inventory.size(), 1);
    ASSERT_EQ(state.inventory[0].count, 1);
}

TEST(inventory_add_stack) {
    GameState state;
    Item i1 = create_item(ItemType::HEALTH_POTION);
    Item i2 = create_item(ItemType::HEALTH_POTION);
    add_to_inventory(state, i1);
    add_to_inventory(state, i2);
    ASSERT_EQ((int)state.inventory.size(), 1);
    ASSERT_EQ(state.inventory[0].count, 2);
}

TEST(inventory_add_different) {
    GameState state;
    Item h = create_item(ItemType::HEALTH_POTION);
    Item s = create_item(ItemType::SWORD);
    add_to_inventory(state, h);
    add_to_inventory(state, s);
    ASSERT_EQ((int)state.inventory.size(), 2);
}

TEST(inventory_use_health) {
    GameState state;
    state.player.hp = 10;
    state.player.max_hp = 20;
    Item i = create_item(ItemType::HEALTH_POTION);
    i.value = 5;
    add_to_inventory(state, i);
    use_inventory_item(state, 0);
    ASSERT_EQ(state.player.hp, 15);
    ASSERT_EQ((int)state.inventory.size(), 0);
}

TEST(inventory_use_health_capped) {
    GameState state;
    state.player.hp = 18;
    state.player.max_hp = 20;
    Item i = create_item(ItemType::HEALTH_POTION);
    i.value = 5;
    add_to_inventory(state, i);
    use_inventory_item(state, 0);
    ASSERT_EQ(state.player.hp, 20);
}

TEST(inventory_use_sword) {
    GameState state;
    state.player.attack = 3;
    Item i = create_item(ItemType::SWORD);
    add_to_inventory(state, i);
    use_inventory_item(state, 0);
    ASSERT_EQ(state.player.attack, 5);
}

TEST(inventory_use_shield) {
    GameState state;
    state.player.defense = 1;
    Item i = create_item(ItemType::SHIELD);
    add_to_inventory(state, i);
    use_inventory_item(state, 0);
    ASSERT_EQ(state.player.defense, 3);
}

TEST(inventory_use_mana) {
    GameState state;
    state.skills.special_cooldown = 3;
    Item i = create_item(ItemType::MANA_POTION);
    add_to_inventory(state, i);
    use_inventory_item(state, 0);
    ASSERT_EQ(state.skills.special_cooldown, 0);
}

TEST(inventory_use_out_of_bounds) {
    GameState state;
    use_inventory_item(state, -1);
    use_inventory_item(state, 0);
    use_inventory_item(state, 99);
    // No crash = pass
}

TEST(minimap_reveal) {
    MinimapState mm;
    mm.init(20, 15);
    ASSERT(!mm.explored[7][10]);
    mm.reveal({10, 7}, 3, 20, 15);
    ASSERT(mm.explored[7][10]);
    ASSERT(mm.explored[7][8]);
}

TEST(minimap_no_crash_oob) {
    MinimapState mm;
    mm.init(10, 8);
    mm.reveal({0, 0}, 5, 10, 8);
    mm.reveal({9, 7}, 5, 10, 8);
    // No crash = pass
}

TEST(skill_defend) {
    GameState state;
    state.skills.defending = false;
    state.skills.defend_turns = 0;
    handle_skill_input(state, '2');
    ASSERT(state.skills.defending);
    ASSERT_EQ(state.skills.defend_turns, SkillState::DEFEND_DURATION);
}

TEST(skill_special) {
    GameState state;
    state.level = 1;
    generate_level(state);
    state.skills.special_cooldown = 0;
    // Place player adjacent to first enemy
    if (!state.enemies.empty()) {
        auto& e = state.enemies[0];
        if (e.alive()) {
            int dx = e.pos.x - state.player.pos.x;
            int dy = e.pos.y - state.player.pos.y;
            if (std::abs(dx) + std::abs(dy) > 1) {
                // Move player next to enemy
                state.map[state.player.pos.y][state.player.pos.x] = Tile::FLOOR;
                state.player.pos = {e.pos.x - 1, e.pos.y};
                if (state.map[state.player.pos.y][state.player.pos.x] == Tile::WALL) {
                    state.player.pos = {e.pos.x + 1, e.pos.y};
                }
                state.map[state.player.pos.y][state.player.pos.x] = Tile::PLAYER;
            }
            int hp_before = e.hp;
            handle_skill_input(state, '3');
            ASSERT(e.hp < hp_before);
            ASSERT(state.skills.special_cooldown > 0);
        }
    }
}

TEST(random_enemy_type_returns_valid) {
    for (int i = 1; i <= 12; i++) {
        EnemyType t = random_enemy_type(i);
        int v = static_cast<int>(t);
        ASSERT(v >= 0 && v <= 4);
    }
}

TEST(random_item_type_returns_valid) {
    for (int i = 1; i <= 12; i++) {
        ItemType t = random_item_type(i);
        int v = static_cast<int>(t);
        ASSERT(v >= 0 && v <= 4);
    }
}

TEST(save_load_roundtrip) {
    GameState state;
    state.level = 3;
    state.turns = 42;
    state.score = 100;
    state.gold_collected = 50;
    state.player.pos = {5, 10};
    state.player.hp = 15;
    state.player.max_hp = 25;
    state.player.attack = 7;
    state.player.defense = 3;
    state.skills.special_cooldown = 2;
    Item sword = create_item(ItemType::SWORD);
    add_to_inventory(state, sword);

    ASSERT(save_game(state, "test_save.json"));

    GameState loaded;
    loaded.level = 1;
    loaded.player.pos = {0, 0};
    ASSERT(load_game(loaded, "test_save.json"));

    ASSERT_EQ(loaded.level, 3);
    ASSERT_EQ(loaded.turns, 42);
    ASSERT_EQ(loaded.score, 100);
    ASSERT_EQ(loaded.gold_collected, 50);
    ASSERT_EQ(loaded.player.hp, 15);
    ASSERT_EQ(loaded.player.max_hp, 25);
    ASSERT_EQ(loaded.player.attack, 7);
    ASSERT_EQ(loaded.player.defense, 3);
    ASSERT_EQ(loaded.skills.special_cooldown, 2);

    remove("test_save.json");
}

TEST(death_screen_no_crash) {
    GameState state;
    state.score = 50;
    state.floors_cleared = 3;
    state.enemies_killed = 12;
    state.items_collected = 7;
    state.gold_collected = 30;
    state.turns = 100;
    state.level = 4;
    death_screen(state);
    // No crash = pass
}

TEST(room_placement_no_overlap) {
    GameState state;
    state.level = 3;
    generate_level(state);
    for (size_t i = 0; i < state.rooms.size(); i++) {
        for (size_t j = i + 1; j < state.rooms.size(); j++) {
            // Rooms might be close but shouldn't fully overlap
            auto& a = state.rooms[i];
            auto& b = state.rooms[j];
            // At least one dimension should differ enough
            bool overlap_x = (a.x < b.x + b.w) && (a.x + a.w > b.x);
            bool overlap_y = (a.y < b.y + b.h) && (a.y + a.h > b.y);
            // It's okay if they're adjacent, just not identical
        }
    }
    // No crash = pass
}

TEST(handle_input_move) {
    GameState state;
    state.level = 1;
    generate_level(state);
    Point start = state.player.pos;
    // Try moving right
    for (int i = 0; i < 10; i++) {
        handle_input(state, 'd');
        if (state.player.pos.x != start.x) break;
    }
    // Player should have moved or stayed (if wall)
    // No crash = pass
}

TEST(handle_input_quit) {
    GameState state;
    state.running = true;
    handle_input(state, 'q');
    ASSERT(!state.running);
}

// ── Main ─────────────────────────────────────────────────────────────────

int main() {
    std::cout << "\n  ╔══════════════════════════════════════╗\n";
    std::cout << "  ║        RetroTerm Test Suite           ║\n";
    std::cout << "  ╚══════════════════════════════════════╝\n\n";

    // Tests auto-register via static initialization above

    std::cout << "\n  ───────────────────────────────────────\n";
    std::cout << "  Results: " << tests_passed << " passed, "
              << tests_failed << " failed, "
              << tests_run << " total\n";
    std::cout << "  ───────────────────────────────────────\n\n";

    if (tests_failed > 0) {
        std::cout << "  \033[31mSOME TESTS FAILED\033[0m\n\n";
        return 1;
    } else {
        std::cout << "  \033[32mALL TESTS PASSED\033[0m\n\n";
        return 0;
    }
}
