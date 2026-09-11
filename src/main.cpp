#include "game.h"
#include <iostream>
#include <fstream>

static void title_screen() {
    std::cout << "\033[H\033[J";
    std::cout << "\033[1;36m";
    std::cout << R"(
  ╔══════════════════════════════════════════════════════════╗
  ║                                                          ║
  ║   ██████╗ ███████╗████████╗██████╗  ██████╗               ║
  ║   ██╔══██╗██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗              ║
  ║   ██████╔╝█████╗     ██║   ██████╔╝██║   ██║              ║
  ║   ██╔══██╗██╔══╝     ██║   ██╔══██╗██║   ██║              ║
  ║   ██║  ██║███████╗   ██║   ██║  ██║╚██████╔╝              ║
  ║   ╚═╝  ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝               ║
  ║                                                          ║
  ║              T  E  R  M                                  ║
  ║                                                          ║
  ║          A Roguelike Dungeon Crawler                     ║
  ║                                                          ║
  ║   [N] New Game    [L] Load Game    [Q] Quit               ║
  ║                                                          ║
  ╚══════════════════════════════════════════════════════════╝
)";
    std::cout << "\033[0m";
}

static bool has_save_file() {
    std::ifstream f("savegame.json");
    return f.good();
}

int main() {
    while (true) {
        title_screen();

        if (has_save_file()) {
            std::cout << "  \033[1;33mSave file detected!\033[0m\n\n";
        }

        char choice;
        std::cin >> choice;

        if (choice == 'q' || choice == 'Q') {
            std::cout << "\n  Farewell, adventurer!\n\n";
            return 0;
        }

        if (choice == 'l' || choice == 'L') {
            if (!has_save_file()) {
                std::cout << "\n  No save file found. Starting new game...\n";
                // fall through to new game
            } else {
                GameState state;
                if (load_game(state, "savegame.json")) {
                    std::cout << "\n  Game loaded! Resuming from Floor " << state.level << "...\n";
                    game_loop(state);
                } else {
                    std::cout << "\n  Failed to load save file. Starting new game...\n";
                }
                continue;
            }
        }

        // New game
        GameState state;
        state.player.name = "Hero";
        game_loop(state);

        std::cout << "\n  Press any key to return to title, or [Q] to quit: ";
        char end_choice;
        std::cin >> end_choice;
        if (end_choice == 'q' || end_choice == 'Q') {
            std::cout << "\n  Farewell, adventurer!\n\n";
            return 0;
        }
    }

    return 0;
}
