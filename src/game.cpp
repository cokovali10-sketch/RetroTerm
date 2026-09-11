#include "game.h"
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <cmath>
#include <fstream>
#include <stack>
#include <queue>
#include <random>

// ── Helpers ──────────────────────────────────────────────────────────────

static std::mt19937 rng{std::random_device{}()};

static int rand_int(int lo, int hi) {
    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(rng);
}

static int manhattan(Point a, Point b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

// ── Entity / Item factories ──────────────────────────────────────────────

Entity create_enemy(EnemyType type, int floor) {
    Entity e;
    e.type = type;
    int scale = 1 + floor / 3;
    switch (type) {
        case EnemyType::GOBLIN:
            e.name = "Goblin";
            e.hp = 6 + scale * 2;  e.max_hp = e.hp;
            e.attack = 2 + scale;   e.defense = 0;
            e.score_value = 10;     e.aggro_range = 8;
            e.can_phase = false;
            break;
        case EnemyType::SKELETON:
            e.name = "Skeleton";
            e.hp = 8 + scale * 3;  e.max_hp = e.hp;
            e.attack = 3 + scale;   e.defense = 1;
            e.score_value = 15;     e.aggro_range = 6;
            e.can_phase = false;
            break;
        case EnemyType::ORC:
            e.name = "Orc";
            e.hp = 14 + scale * 4; e.max_hp = e.hp;
            e.attack = 4 + scale;   e.defense = 2;
            e.score_value = 25;     e.aggro_range = 5;
            e.can_phase = false;
            break;
        case EnemyType::DRAGON:
            e.name = "Dragon";
            e.hp = 20 + scale * 6; e.max_hp = e.hp;
            e.attack = 6 + scale;   e.defense = 3;
            e.score_value = 50;     e.aggro_range = 10;
            e.can_phase = false;
            break;
        case EnemyType::GHOST:
            e.name = "Ghost";
            e.hp = 10 + scale * 3; e.max_hp = e.hp;
            e.attack = 3 + scale;   e.defense = 0;
            e.score_value = 20;     e.aggro_range = 7;
            e.can_phase = true;
            break;
    }
    return e;
}

Item create_item(ItemType type) {
    Item item;
    item.type = type;
    switch (type) {
        case ItemType::HEALTH_POTION:
            item.name = "Health Potion";
            item.value = 10 + rand_int(0, 5);
            break;
        case ItemType::MANA_POTION:
            item.name = "Mana Potion";
            item.value = 0;
            break;
        case ItemType::SWORD:
            item.name = "Sword";
            item.value = 2;
            break;
        case ItemType::SHIELD:
            item.name = "Shield";
            item.value = 2;
            break;
        case ItemType::KEY:
            item.name = "Key";
            item.value = 1;
            break;
    }
    return item;
}

ItemType random_item_type(int floor) {
    int r = rand_int(0, 99);
    if (r < 35) return ItemType::HEALTH_POTION;
    if (r < 50) return ItemType::MANA_POTION;
    if (r < 65) return ItemType::SWORD;
    if (r < 80) return ItemType::SHIELD;
    return ItemType::KEY;
}

EnemyType random_enemy_type(int floor) {
    if (floor <= 2) {
        int r = rand_int(0, 99);
        if (r < 60) return EnemyType::GOBLIN;
        return EnemyType::SKELETON;
    }
    if (floor <= 5) {
        int r = rand_int(0, 99);
        if (r < 30) return EnemyType::GOBLIN;
        if (r < 60) return EnemyType::SKELETON;
        if (r < 85) return EnemyType::ORC;
        return EnemyType::GHOST;
    }
    int r = rand_int(0, 99);
    if (r < 15) return EnemyType::GOBLIN;
    if (r < 35) return EnemyType::SKELETON;
    if (r < 55) return EnemyType::ORC;
    if (r < 80) return EnemyType::DRAGON;
    return EnemyType::GHOST;
}

// ── Dungeon Generation (BSP) ────────────────────────────────────────────

static bool rooms_overlap(const DungeonRoom& a, const DungeonRoom& b, int pad = 2) {
    return a.x - pad < b.x + b.w + pad && a.x + a.w + pad > b.x - pad &&
           a.y - pad < b.y + b.h + pad && a.y + a.h + pad > b.y - pad;
}

static void carve_room(std::vector<std::vector<Tile>>& map, const DungeonRoom& r) {
    for (int y = r.y; y < r.y + r.h; y++)
        for (int x = r.x; x < r.x + r.w; x++)
            map[y][x] = Tile::FLOOR;
}

static void carve_h_corridor(std::vector<std::vector<Tile>>& map, int x1, int x2, int y) {
    int lo = std::min(x1, x2), hi = std::max(x1, x2);
    for (int x = lo; x <= hi; x++)
        if (map[y][x] == Tile::WALL) map[y][x] = Tile::CORRIDOR;
}

static void carve_v_corridor(std::vector<std::vector<Tile>>& map, int y1, int y2, int x) {
    int lo = std::min(y1, y2), hi = std::max(y1, y2);
    for (int y = lo; y <= hi; y++)
        if (map[y][x] == Tile::WALL) map[y][x] = Tile::CORRIDOR;
}

static void carve_corridor(std::vector<std::vector<Tile>>& map, Point a, Point b) {
    if (rand_int(0, 1) == 0) {
        carve_h_corridor(map, a.x, b.x, a.y);
        carve_v_corridor(map, a.y, b.y, b.x);
    } else {
        carve_v_corridor(map, a.y, b.y, a.x);
        carve_h_corridor(map, a.x, b.x, b.y);
    }
}

static std::vector<DungeonRoom> generate_bsp_rooms(int map_w, int map_h, int level) {
    struct BSPNode {
        int x, y, w, h;
        BSPNode* left = nullptr;
        BSPNode* right = nullptr;
        DungeonRoom room;
        bool has_room = false;
    };

    std::vector<BSPNode> nodes;
    nodes.reserve(64);

    std::stack<BSPNode*> splits;
    BSPNode root{1, 1, map_w - 2, map_h - 2, nullptr, nullptr, {}, false};
    nodes.push_back(root);
    splits.push(&nodes.back());

    int target_leaves = 5 + std::min(level, 6);

    while (!splits.empty()) {
        BSPNode* node = splits.top();
        splits.pop();

        if ((node->w < 10 || node->h < 8) || 
            (count_if(nodes.begin(), nodes.end(), [](const BSPNode& n) {
                return n.left == nullptr && n.right == nullptr;
            }) >= target_leaves)) {
            continue;
        }

        bool split_h = (node->w > node->h) ? false :
                       (node->h > node->w) ? true :
                       (rand_int(0, 1) == 0);

        if (split_h && node->h >= 8) {
            int split = rand_int(3, node->h - 3);
            BSPNode left{node->x, node->y, node->w, split};
            BSPNode right{node->x, node->y + split, node->w, node->h - split};
            nodes.push_back(left); node->left = &nodes.back();
            nodes.push_back(right); node->right = &nodes.back();
            splits.push(node->left);
            splits.push(node->right);
        } else if (!split_h && node->w >= 10) {
            int split = rand_int(4, node->w - 4);
            BSPNode left{node->x, node->y, split, node->h};
            BSPNode right{node->x + split, node->y, node->w - split, node->h};
            nodes.push_back(left); node->left = &nodes.back();
            nodes.push_back(right); node->right = &nodes.back();
            splits.push(node->left);
            splits.push(node->right);
        }
    }

    // Create rooms in leaf nodes
    std::vector<DungeonRoom> rooms;
    for (auto& node : nodes) {
        if (node.left == nullptr && node.right == nullptr) {
            int rw = rand_int(3, std::max(3, node.w - 2));
            int rh = rand_int(3, std::max(3, node.h - 2));
            int rx = node.x + rand_int(0, std::max(0, node.w - rw));
            int ry = node.y + rand_int(0, std::max(0, node.h - rh));
            DungeonRoom room{rx, ry, rw, rh};
            node.room = room;
            node.has_room = true;
            rooms.push_back(room);
        }
    }

    // Connect rooms via BSP tree
    std::vector<std::vector<Tile>> temp(map_h, std::vector<Tile>(map_w, Tile::WALL));
    for (auto& r : rooms) carve_room(temp, r);

    // Connect siblings
    for (auto& node : nodes) {
        if (node.left && node.right) {
            // Find a room in each subtree
            auto find_room = [](BSPNode* n, auto& self) -> DungeonRoom* {
                if (!n) return nullptr;
                if (n->has_room) return &n->room;
                DungeonRoom* l = self(n->left, self);
                if (l) return l;
                return self(n->right, self);
            };
            DungeonRoom* lr = find_room(node.left, find_room);
            DungeonRoom* rr = find_room(node.right, find_room);
            if (lr && rr) {
                Point a{lr->cx(), lr->cy()};
                Point b{rr->cx(), rr->cy()};
                carve_corridor(temp, a, b);
            }
        }
    }

    // Copy walls back
    for (int y = 0; y < map_h; y++)
        for (int x = 0; x < map_w; x++)
            if (temp[y][x] == Tile::WALL)
                temp[y][x] = Tile::WALL;

    return rooms;
}

// ── Map init & level generation ──────────────────────────────────────────

void init_map(GameState& state) {
    state.map.assign(state.height, std::vector<Tile>(state.width, Tile::WALL));
}

void generate_level(GameState& state) {
    init_map(state);

    state.rooms = generate_bsp_rooms(state.width, state.height, state.level);

    for (auto& r : state.rooms)
        carve_room(state.map, r);

    // Player spawn in first room
    if (!state.rooms.empty()) {
        auto& first = state.rooms.front();
        state.player.pos = {first.cx(), first.cy()};
        state.map[first.cy()][first.cx()] = Tile::PLAYER;
    }

    // Stairs in last room
    if (state.rooms.size() > 1) {
        auto& last = state.rooms.back();
        state.map[last.cy()][last.cx()] = Tile::STAIRS;
    }

    // Enemies
    state.enemies.clear();
    int num_enemies = 3 + state.level * 2;
    for (int i = 0; i < num_enemies; i++) {
        EnemyType etype = random_enemy_type(state.level);
        Entity e = create_enemy(etype, state.level);
        place_entity(state, e);
        state.enemies.push_back(e);
    }

    // Items
    state.items.clear();
    int num_items = 3 + state.level;
    for (int i = 0; i < num_items; i++) {
        ItemType itype = random_item_type(state.level);
        Item item = create_item(itype);
        place_item(state, item);
        state.items.push_back(item);
    }

    // Reveal minimap for new floor
    state.minimap.init(state.width, state.height);
    state.minimap.reveal(state.player.pos, 3, state.width, state.height);

    state.message = "Floor " + std::to_string(state.level) + " — Find the stairs!";
}

void place_entity(GameState& state, Entity& e) {
    for (int attempt = 0; attempt < 1000; attempt++) {
        if (state.rooms.empty()) break;
        int ri = rand_int(0, (int)state.rooms.size() - 1);
        auto& r = state.rooms[ri];
        int x = rand_int(r.x, r.x + r.w - 1);
        int y = rand_int(r.y, r.y + r.h - 1);
        if (state.map[y][x] == Tile::FLOOR) {
            e.pos = {x, y};
            state.map[y][x] = Tile::ITEM; // placeholder until render
            return;
        }
    }
    e.pos = {state.width / 2, state.height / 2};
}

void place_item(GameState& state, Item& item) {
    for (int attempt = 0; attempt < 1000; attempt++) {
        if (state.rooms.empty()) break;
        int ri = rand_int(0, (int)state.rooms.size() - 1);
        auto& r = state.rooms[ri];
        int x = rand_int(r.x, r.x + r.w - 1);
        int y = rand_int(r.y, r.y + r.h - 1);
        if (state.map[y][x] == Tile::FLOOR) {
            item.pos = {x, y};
            state.map[y][x] = Tile::ITEM;
            return;
        }
    }
}

// ── Line of sight ────────────────────────────────────────────────────────

bool line_of_sight(const GameState& state, Point from, Point to) {
    int dx = std::abs(to.x - from.x);
    int dy = std::abs(to.y - from.y);
    int sx = from.x < to.x ? 1 : -1;
    int sy = from.y < to.y ? 1 : -1;
    int err = dx - dy;
    int x = from.x, y = from.y;
    while (true) {
        if (x == to.x && y == to.y) return true;
        if (state.map[y][x] == Tile::WALL) return false;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 < dx)  { err += dx; y += sy; }
    }
}

// ── Collision ────────────────────────────────────────────────────────────

bool check_collision(const GameState& state, Point pos) {
    if (pos.x < 0 || pos.x >= state.width || pos.y < 0 || pos.y >= state.height)
        return true;
    Tile t = state.map[pos.y][pos.x];
    return t == Tile::WALL || t == Tile::LOCKED;
}

// ── Inventory ────────────────────────────────────────────────────────────

void add_to_inventory(GameState& state, const Item& item) {
    for (auto& slot : state.inventory) {
        if (slot.item.type == item.type) {
            slot.count++;
            return;
        }
    }
    InventorySlot slot;
    slot.item = item;
    slot.count = 1;
    state.inventory.push_back(slot);
}

void use_inventory_item(GameState& state, int index) {
    if (index < 0 || index >= (int)state.inventory.size()) return;
    auto& slot = state.inventory[index];
    if (slot.count <= 0) return;

    Item& item = slot.item;
    switch (item.type) {
        case ItemType::HEALTH_POTION:
            if (state.player.hp >= state.player.max_hp) {
                state.message = "Already at full HP!";
                return;
            }
            state.player.hp = std::min(state.player.hp + item.value, state.player.max_hp);
            state.message = "Used Health Potion! +" + std::to_string(item.value) + " HP";
            break;
        case ItemType::MANA_POTION:
            state.skills.special_cooldown = 0;
            state.message = "Used Mana Potion! Special ready.";
            break;
        case ItemType::SWORD:
            state.player.attack += item.value;
            state.message = "Equipped Sword! ATK +" + std::to_string(item.value);
            break;
        case ItemType::SHIELD:
            state.player.defense += item.value;
            state.message = "Equipped Shield! DEF +" + std::to_string(item.value);
            break;
        case ItemType::KEY:
            state.message = "Key added to keyring.";
            break;
    }

    slot.count--;
    if (slot.count <= 0) {
        state.inventory.erase(state.inventory.begin() + index);
    }
}

void open_inventory(GameState& state) {
    (void)state;
}

// ── Rendering ────────────────────────────────────────────────────────────

void render_minimap(const GameState& state) {
    std::cout << "\033[H\033[J";
    std::cout << "  ╔══════════════════════════════════════════════════╗\n";
    std::cout << "  ║                   M I N I M A P                  ║\n";
    std::cout << "  ╠══════════════════════════════════════════════════╣\n";

    int scale_x = 2, scale_y = 1;
    for (int y = 0; y < state.height; y++) {
        std::cout << "  ║ ";
        for (int x = 0; x < state.width; x++) {
            if (!state.minimap.explored[y][x]) {
                std::cout << "  ";
                continue;
            }
            Point p{x, y};
            if (p == state.player.pos) {
                std::cout << "\033[1;32m@@\033[0m";
            } else if (state.map[y][x] == Tile::STAIRS) {
                std::cout << "\033[1;36m>>\033[0m";
            } else if (state.map[y][x] == Tile::WALL) {
                std::cout << "\033[90m##\033[0m";
            } else if (state.map[y][x] == Tile::FLOOR || state.map[y][x] == Tile::CORRIDOR) {
                std::cout << "\033[37m..\033[0m";
            } else {
                std::cout << "  ";
            }
        }
        std::cout << "║\n";
    }

    std::cout << "  ╠══════════════════════════════════════════════════╣\n";
    std::cout << "  ║  [F1] Close Minimap                             ║\n";
    std::cout << "  ╚══════════════════════════════════════════════════╝\n";
}

void render(const GameState& state) {
    std::cout << "\033[H\033[J";
    std::cout << "  ╔════════════════════════════════════════════════════════╗\n";
    std::cout << "  ║              R E T R O   T E R M                      ║\n";
    std::cout << "  ╠════════════════════════════════════════════════════════╣\n";

    for (int y = 0; y < state.height; y++) {
        std::cout << "  ║ ";
        for (int x = 0; x < state.width; x++) {
            Point p{x, y};
            // Check if player is here
            if (p == state.player.pos) {
                std::cout << "\033[1;32m@\033[0m ";
                continue;
            }
            // Check enemies
            bool drawn = false;
            for (auto& e : state.enemies) {
                if (e.alive() && e.pos == p) {
                    char sym;
                    switch (e.type) {
                        case EnemyType::GOBLIN:   sym = 'G'; break;
                        case EnemyType::SKELETON:  sym = 'S'; break;
                        case EnemyType::ORC:       sym = 'O'; break;
                        case EnemyType::DRAGON:    sym = 'D'; break;
                        case EnemyType::GHOST:     sym = 'W'; break;
                        default:                   sym = 'E'; break;
                    }
                    std::cout << "\033[1;31m" << sym << "\033[0m ";
                    drawn = true;
                    break;
                }
            }
            if (drawn) continue;

            // Check items
            for (auto& i : state.items) {
                if (!i.collected && i.pos == p) {
                    std::cout << "\033[1;33m*\033[0m ";
                    drawn = true;
                    break;
                }
            }
            if (drawn) continue;

            // Tiles
            Tile t = state.map[y][x];
            switch (t) {
                case Tile::WALL:     std::cout << "\033[47m  \033[0m"; break;
                case Tile::FLOOR:    std::cout << "\033[37m. \033[0m"; break;
                case Tile::CORRIDOR: std::cout << "\033[90m, \033[0m"; break;
                case Tile::DOOR:     std::cout << "\033[33m+ \033[0m"; break;
                case Tile::LOCKED:   std::cout << "\033[31m%%\033[0m"; break;
                case Tile::STAIRS:   std::cout << "\033[1;36m> \033[0m"; break;
                default:             std::cout << "  "; break;
            }
        }
        std::cout << "║\n";
    }

    std::cout << "  ╠════════════════════════════════════════════════════════╣\n";

    // Status bar
    std::ostringstream status;
    status << "  ║ HP: " << state.player.hp << "/" << state.player.max_hp
           << "  ATK: " << state.player.attack
           << "  DEF: " << state.player.defense
           << "  Gold: " << state.gold_collected
           << "  Floor: " << state.level
           << "  Turn: " << state.turns;
    std::string s = status.str();
    while ((int)s.size() < 60) s += ' ';
    s += "║";
    std::cout << s << "\n";

    // Skill bar
    std::ostringstream skill_bar;
    skill_bar << "  ║ [F2] Defend";
    if (state.skills.defending)
        skill_bar << " (active:" << state.skills.defend_turns << ")";
    else
        skill_bar << "              ";
    skill_bar << "  [F3] Special";
    if (state.skills.special_cooldown > 0)
        skill_bar << " (cd:" << state.skills.special_cooldown << ")";
    else
        skill_bar << " (ready)     ";
    skill_bar << "  [I] Inv  [F1] Map  [K] Save  [Q] Quit";
    std::string sb = skill_bar.str();
    while ((int)sb.size() < 60) sb += ' ';
    sb += "║";
    std::cout << sb << "\n";

    if (!state.message.empty()) {
        std::string msg = "  ║ " + state.message;
        while ((int)msg.size() < 60) msg += ' ';
        msg += "║";
        std::cout << msg << "\n";
    }

    std::cout << "  ╚════════════════════════════════════════════════════════╝\n";
}

// ── Input ────────────────────────────────────────────────────────────────

void handle_skill_input(GameState& state, char input) {
    switch (input) {
        case '2': // Defend
            if (!state.skills.defending) {
                state.skills.defending = true;
                state.skills.defend_turns = SkillState::DEFEND_DURATION;
                state.message = "You brace yourself! Damage halved for 3 turns.";
            } else {
                state.message = "Already defending!";
            }
            break;
        case '3': // Special
            if (state.skills.special_cooldown > 0) {
                state.message = "Special on cooldown! (" + std::to_string(state.skills.special_cooldown) + " turns)";
                break;
            }
            // Find nearest enemy and hit it
            {
                Entity* nearest = nullptr;
                int best_dist = 999;
                for (auto& e : state.enemies) {
                    if (!e.alive()) continue;
                    int d = manhattan(state.player.pos, e.pos);
                    if (d < best_dist && d <= 3) {
                        best_dist = d;
                        nearest = &e;
                    }
                }
                if (nearest) {
                    int dmg = state.player.attack * SkillState::SPECIAL_MULTIPLIER - nearest->defense;
                    if (dmg < 1) dmg = 1;
                    nearest->hp -= dmg;
                    std::ostringstream oss;
                    oss << "Special attack! " << nearest->name << " takes " << dmg << " damage!";
                    state.message = oss.str();
                    if (!nearest->alive()) {
                        state.score += nearest->score_value;
                        state.enemies_killed++;
                        state.map[nearest->pos.y][nearest->pos.x] = Tile::FLOOR;
                        state.message += " Enemy defeated!";
                    }
                    state.skills.special_cooldown = SkillState::SPECIAL_MAX_COOLDOWN;
                } else {
                    state.message = "No enemy in range for Special!";
                }
            }
            break;
    }
}

void handle_input(GameState& state, char input) {
    // Skill keys
    if (input == '2' || input == '3') {
        handle_skill_input(state, input);
        return;
    }

    // Inventory use (1-9)
    if (input >= '1' && input <= '9') {
        int idx = input - '1';
        use_inventory_item(state, idx);
        return;
    }

    Point new_pos = state.player.pos;
    switch (input) {
        case 'w': case 'W': new_pos.y--; break;
        case 's': case 'S': new_pos.y++; break;
        case 'a': case 'A': new_pos.x--; break;
        case 'd': case 'D': new_pos.x++; break;
        case 'f': case 'F': // F1 handled in game_loop
        case '1': // F1 also handled in game_loop
            return;
        case '>': {
            if (state.map[state.player.pos.y][state.player.pos.x] == Tile::STAIRS) {
                state.level++;
                state.floors_cleared++;
                if (state.level > state.max_level) {
                    state.message = "You conquered all floors! You WIN!";
                    state.score += 500;
                    state.running = false;
                } else {
                    generate_level(state);
                }
            } else {
                state.message = "No stairs here.";
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

    // Enemy collision (combat)
    for (auto& enemy : state.enemies) {
        if (enemy.alive() && enemy.pos == new_pos) {
            int dmg = state.player.attack - enemy.defense;
            if (dmg < 1) dmg = 1;
            enemy.hp -= dmg;
            std::ostringstream oss;
            oss << "You hit " << enemy.name << " for " << dmg << " damage!";
            state.message = oss.str();
            if (!enemy.alive()) {
                state.score += enemy.score_value;
                state.enemies_killed++;
                state.map[new_pos.y][new_pos.x] = Tile::FLOOR;
                state.message += " Enemy defeated! (+" + std::to_string(enemy.score_value) + " pts)";
            }
            state.turns++;
            return;
        }
    }

    // Item pickup
    for (auto& item : state.items) {
        if (!item.collected && item.pos == new_pos) {
            item.collected = true;
            state.items_collected++;
            state.map[new_pos.y][new_pos.x] = Tile::FLOOR;

            switch (item.type) {
                case ItemType::HEALTH_POTION:
                case ItemType::MANA_POTION:
                case ItemType::KEY:
                    add_to_inventory(state, item);
                    state.message = "Picked up " + item.name + "!";
                    break;
                case ItemType::SWORD:
                    add_to_inventory(state, item);
                    state.message = "Found a Sword! Press [I] then [5] to equip.";
                    break;
                case ItemType::SHIELD:
                    add_to_inventory(state, item);
                    state.message = "Found a Shield! Press [I] then [5] to equip.";
                    break;
            }
        }
    }

    // Move player
    state.map[state.player.pos.y][state.player.pos.x] = Tile::FLOOR;
    state.player.pos = new_pos;
    state.map[new_pos.y][new_pos.x] = Tile::PLAYER;
    state.turns++;

    // Update minimap
    state.minimap.reveal(state.player.pos, 4, state.width, state.height);

    // Decrease cooldowns
    if (state.skills.special_cooldown > 0) state.skills.special_cooldown--;
    if (state.skills.defending) {
        state.skills.defend_turns--;
        if (state.skills.defend_turns <= 0) {
            state.skills.defending = false;
            state.message += " Defend wore off.";
        }
    }
}

// ── Enemy AI ─────────────────────────────────────────────────────────────

void update_enemies(GameState& state) {
    for (auto& enemy : state.enemies) {
        if (!enemy.alive()) continue;

        int dist = manhattan(state.player.pos, enemy.pos);

        // Adjacent — attack
        if (dist == 1) {
            int dmg = enemy.attack - state.player.defense;
            if (dmg < 1) dmg = 1;
            if (state.skills.defending) dmg = (dmg + 1) / 2;

            // Ghost special: 30% chance to ignore defense
            if (enemy.type == EnemyType::GHOST && rand_int(0, 99) < 30) {
                dmg = enemy.attack;
                if (state.skills.defending) dmg = (dmg + 1) / 2;
                state.message = enemy.name + " phases through your defense for " + std::to_string(dmg) + "!";
            } else {
                state.message = enemy.name + " attacks for " + std::to_string(dmg) + " damage!";
            }

            state.player.hp -= dmg;
            if (state.player.hp <= 0) {
                state.player.hp = 0;
                state.dead = true;
                state.running = false;
            }
            continue;
        }

        // Line of sight check
        if (dist > enemy.aggro_range) continue;
        if (!line_of_sight(state, enemy.pos, state.player.pos)) continue;

        // Orc rage: double damage when below 50% HP
        if (enemy.type == EnemyType::ORC && !enemy.enraged &&
            enemy.hp <= enemy.max_hp / 2) {
            enemy.enraged = true;
            enemy.attack *= 2;
        }

        // Movement — greedy chase
        Point new_pos = enemy.pos;
        int dx = state.player.pos.x - enemy.pos.x;
        int dy = state.player.pos.y - enemy.pos.y;

        // Dragon: ranged breath attack (2 tiles)
        if (enemy.type == EnemyType::DRAGON && dist <= 2 && dist > 1) {
            int dmg = enemy.attack - state.player.defense;
            if (dmg < 1) dmg = 1;
            if (state.skills.defending) dmg = (dmg + 1) / 2;
            state.player.hp -= dmg;
            state.message = enemy.name + " breathes fire for " + std::to_string(dmg) + "!";
            if (state.player.hp <= 0) {
                state.player.hp = 0;
                state.dead = true;
                state.running = false;
            }
            continue;
        }

        // Choose direction
        if (std::abs(dx) > std::abs(dy)) {
            new_pos.x += (dx > 0) ? 1 : -1;
        } else if (std::abs(dy) > std::abs(dx)) {
            new_pos.y += (dy > 0) ? 1 : -1;
        } else {
            // Tie-break randomly
            if (rand_int(0, 1) == 0) new_pos.x += (dx > 0) ? 1 : -1;
            else new_pos.y += (dy > 0) ? 1 : -1;
        }

        bool can_move = false;
        if (enemy.can_phase) {
            can_move = (new_pos.x >= 0 && new_pos.x < state.width &&
                        new_pos.y >= 0 && new_pos.y < state.height &&
                        state.map[new_pos.y][new_pos.x] != Tile::WALL);
        } else {
            can_move = !check_collision(state, new_pos);
        }

        if (can_move && state.map[new_pos.y][new_pos.x] != Tile::PLAYER) {
            // Ghost can teleport occasionally
            if (enemy.type == EnemyType::GHOST && rand_int(0, 99) < 10) {
                for (int attempt = 0; attempt < 20; attempt++) {
                    int tx = state.player.pos.x + rand_int(-3, 3);
                    int ty = state.player.pos.y + rand_int(-3, 3);
                    if (tx >= 0 && tx < state.width && ty >= 0 && ty < state.height &&
                        state.map[ty][tx] == Tile::FLOOR) {
                        state.map[enemy.pos.y][enemy.pos.x] = Tile::FLOOR;
                        enemy.pos = {tx, ty};
                        state.map[ty][tx] = Tile::ITEM; // enemy placeholder
                        break;
                    }
                }
            } else {
                state.map[enemy.pos.y][enemy.pos.x] = Tile::FLOOR;
                enemy.pos = new_pos;
                state.map[new_pos.y][new_pos.x] = Tile::ITEM; // enemy placeholder
            }
        }
    }
}

// ── Death Screen ─────────────────────────────────────────────────────────

void death_screen(const GameState& state) {
    std::cout << "\033[H\033[J";
    std::cout << "\033[1;31m";
    std::cout << "  ╔══════════════════════════════════════════════════╗\n";
    std::cout << "  ║                  Y O U   D I E D                 ║\n";
    std::cout << "  ╠══════════════════════════════════════════════════╣\n";
    std::cout << "  ║                                                  ║\n";
    std::cout << "  ║   Final Score:     " << state.score;
    std::cout << "                            ║\n";
    std::cout << "  ║   Floors Cleared:  " << state.floors_cleared;
    std::cout << "                            ║\n";
    std::cout << "  ║   Enemies Killed:  " << state.enemies_killed;
    std::cout << "                            ║\n";
    std::cout << "  ║   Items Collected: " << state.items_collected;
    std::cout << "                            ║\n";
    std::cout << "  ║   Gold Gathered:   " << state.gold_collected;
    std::cout << "                            ║\n";
    std::cout << "  ║   Turns Survived:  " << state.turns;
    std::cout << "                            ║\n";
    std::cout << "  ║   Floor Reached:   " << state.level;
    std::cout << "                            ║\n";
    std::cout << "  ║                                                  ║\n";
    std::cout << "  ╚══════════════════════════════════════════════════╝\n";
    std::cout << "\033[0m\n";
}

// ── Save / Load (JSON) ───────────────────────────────────────────────────

static void write_json_string(std::ofstream& f, const std::string& key, const std::string& val) {
    f << "    \"" << key << "\": \"" << val << "\"";
}

template<typename T>
static void write_json_int(std::ofstream& f, const std::string& key, T val) {
    f << "    \"" << key << "\": " << val;
}

bool save_game(const GameState& state, const std::string& filename) {
    std::ofstream f(filename);
    if (!f.is_open()) return false;

    f << "{\n";
    write_json_int(f, "level", state.level); f << ",\n";
    write_json_int(f, "turns", state.turns); f << ",\n";
    write_json_int(f, "score", state.score); f << ",\n";
    write_json_int(f, "gold", state.gold_collected); f << ",\n";
    write_json_int(f, "kills", state.enemies_killed); f << ",\n";
    write_json_int(f, "items_collected", state.items_collected); f << ",\n";
    write_json_int(f, "floors_cleared", state.floors_cleared); f << ",\n";

    // Player
    f << "  \"player\": {\n";
    write_json_int(f, "x", state.player.pos.x); f << ",\n";
    write_json_int(f, "y", state.player.pos.y); f << ",\n";
    write_json_int(f, "hp", state.player.hp); f << ",\n";
    write_json_int(f, "max_hp", state.player.max_hp); f << ",\n";
    write_json_int(f, "attack", state.player.attack); f << ",\n";
    write_json_int(f, "defense", state.player.defense); f << ",\n";
    write_json_string(f, "name", state.player.name); f << "\n";
    f << "  },\n";

    // Skills
    f << "  \"skills\": {\n";
    write_json_int(f, "defending", state.skills.defending ? 1 : 0); f << ",\n";
    write_json_int(f, "defend_turns", state.skills.defend_turns); f << ",\n";
    write_json_int(f, "special_cooldown", state.skills.special_cooldown); f << "\n";
    f << "  },\n";

    // Inventory
    f << "  \"inventory\": [\n";
    for (int i = 0; i < (int)state.inventory.size(); i++) {
        auto& slot = state.inventory[i];
        f << "    { \"type\": " << static_cast<int>(slot.item.type)
          << ", \"name\": \"" << slot.item.name
          << "\", \"value\": " << slot.item.value
          << ", \"count\": " << slot.count << " }";
        if (i < (int)state.inventory.size() - 1) f << ",";
        f << "\n";
    }
    f << "  ],\n";

    // Explored tiles (RLE)
    f << "  \"explored\": [\n";
    for (int y = 0; y < state.height; y++) {
        f << "    [";
        for (int x = 0; x < state.width; x++) {
            f << (state.minimap.explored[y][x] ? 1 : 0);
            if (x < state.width - 1) f << ",";
        }
        f << "]";
        if (y < state.height - 1) f << ",";
        f << "\n";
    }
    f << "  ]\n";

    f << "}\n";
    f.close();
    return true;
}

static std::string json_read_token(std::ifstream& f) {
    std::string token;
    char c;
    while (f.get(c)) {
        if (c == '"') {
            while (f.get(c) && c != '"') token += c;
            return token;
        }
        if (c == '{' || c == '}' || c == '[' || c == ']' || c == ':') {
            token += c;
            return token;
        }
        if (c == ',' || c == ' ' || c == '\n' || c == '\r' || c == '\t') continue;
        // Number
        if (c == '-' || (c >= '0' && c <= '9')) {
            token += c;
            while (f.get(c) && ((c >= '0' && c <= '9') || c == '-')) token += c;
            return token;
        }
    }
    return token;
}

static int json_read_int(std::ifstream& f) {
    std::string token = json_read_token(f);
    // skip colon
    std::string next = json_read_token(f);
    if (next == ":") next = json_read_token(f);
    try { return std::stoi(next); } catch (...) { return 0; }
}

static std::string json_read_string_val(std::ifstream& f) {
    std::string token = json_read_token(f);
    std::string next = json_read_token(f);
    if (next == ":") next = json_read_token(f);
    return next;
}

bool load_game(GameState& state, const std::string& filename) {
    std::ifstream f(filename);
    if (!f.is_open()) return false;

    // Simple JSON parser — reads key-value pairs
    std::string key;
    while (true) {
        key = json_read_token(f);
        if (key.empty() || key == "}") break;

        if (key == "level") { state.level = json_read_int(f); }
        else if (key == "turns") { state.turns = json_read_int(f); }
        else if (key == "score") { state.score = json_read_int(f); }
        else if (key == "gold") { state.gold_collected = json_read_int(f); }
        else if (key == "kills") { state.enemies_killed = json_read_int(f); }
        else if (key == "items_collected") { state.items_collected = json_read_int(f); }
        else if (key == "floors_cleared") { state.floors_cleared = json_read_int(f); }
        else if (key == "player") {
            // skip {
            json_read_token(f);
            while (true) {
                std::string pk = json_read_token(f);
                if (pk.empty() || pk == "}") break;
                if (pk == "x") { state.player.pos.x = json_read_int(f); }
                else if (pk == "y") { state.player.pos.y = json_read_int(f); }
                else if (pk == "hp") { state.player.hp = json_read_int(f); }
                else if (pk == "max_hp") { state.player.max_hp = json_read_int(f); }
                else if (pk == "attack") { state.player.attack = json_read_int(f); }
                else if (pk == "defense") { state.player.defense = json_read_int(f); }
                else if (pk == "name") { state.player.name = json_read_string_val(f); }
                else { json_read_token(f); } // skip value
            }
        }
        else if (key == "skills") {
            json_read_token(f);
            while (true) {
                std::string sk = json_read_token(f);
                if (sk.empty() || sk == "}") break;
                if (sk == "defending") { state.skills.defending = json_read_int(f) != 0; }
                else if (sk == "defend_turns") { state.skills.defend_turns = json_read_int(f); }
                else if (sk == "special_cooldown") { state.skills.special_cooldown = json_read_int(f); }
                else { json_read_token(f); }
            }
        }
        else if (key == "inventory") {
            json_read_token(f); // [
            while (true) {
                std::string tk = json_read_token(f);
                if (tk.empty() || tk == "]") break;
                if (tk == "{") {
                    InventorySlot slot;
                    while (true) {
                        std::string ik = json_read_token(f);
                        if (ik.empty() || ik == "}") break;
                        if (ik == "type") { slot.item.type = static_cast<ItemType>(json_read_int(f)); }
                        else if (ik == "name") { slot.item.name = json_read_string_val(f); }
                        else if (ik == "value") { slot.item.value = json_read_int(f); }
                        else if (ik == "count") { slot.count = json_read_int(f); }
                        else { json_read_token(f); }
                    }
                    state.inventory.push_back(slot);
                }
            }
        }
        else if (key == "explored") {
            state.minimap.init(state.width, state.height);
            json_read_token(f); // [
            for (int y = 0; y < state.height; y++) {
                json_read_token(f); // [
                for (int x = 0; x < state.width; x++) {
                    std::string val = json_read_token(f);
                    state.minimap.explored[y][x] = (val == "1");
                    json_read_token(f); // comma
                }
                json_read_token(f); // ]
                json_read_token(f); // comma
            }
            json_read_token(f); // ]
        }
        else {
            json_read_token(f); // skip unknown value
        }
    }

    f.close();

    // Regenerate the level map (we only saved metadata)
    state.running = true;
    state.dead = false;
    state.enemies.clear();
    state.items.clear();
    generate_level(state);

    // Restore player position (override generate_level's default)
    state.map[state.player.pos.y][state.player.pos.x] = Tile::FLOOR;
    state.player.pos = {state.player.pos.x, state.player.pos.y};
    state.map[state.player.pos.y][state.player.pos.x] = Tile::PLAYER;

    return true;
}

// ── Game Loop ────────────────────────────────────────────────────────────

void game_loop(GameState& state) {
    std::srand(std::time(nullptr));
    generate_level(state);
    state.player.name = "Hero";
    state.player.hp = 20;
    state.player.max_hp = 20;
    state.player.attack = 3;
    state.player.defense = 1;

    while (state.running) {
        render(state);

        char input;
        std::cin >> input;

        // F1 toggle minimap
        if (input == '1') {
            state.minimap.visible = !state.minimap.visible;
            continue;
        }

        if (state.minimap.visible) {
            if (input == '1' || input == '\033') {
                state.minimap.visible = false;
            }
            continue;
        }

        // Save
        if (input == 'k' || input == 'K') {
            if (save_game(state, "savegame.json")) {
                state.message = "Game saved to savegame.json!";
            } else {
                state.message = "Failed to save game!";
            }
            continue;
        }

        handle_input(state, input);
        if (state.running) {
            update_enemies(state);
        }
    }

    if (state.dead) {
        death_screen(state);
    } else {
        render(state);
        std::cout << "\n  Thanks for playing RetroTerm!\n";
        std::cout << "  Final Score: " << state.score << "\n";
        std::cout << "  Floors cleared: " << state.floors_cleared << "\n";
        std::cout << "  Enemies slain: " << state.enemies_killed << "\n\n";
    }
}
