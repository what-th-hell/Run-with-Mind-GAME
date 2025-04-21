#ifndef GAME_H
#define GAME_H

#include <vector>
#include <string>
#include "Entity.h"

// The Game structure holds the entire game state.
struct Game {
    std::vector<std::vector<char>> grid; // The maze grid.
    Player player;                       // The player.
    std::vector<Enemy> enemies;          // List of enemy objects.
    std::vector<Position> powerups;      // Positions of collectible items.
    Position exitPos;                    // Position of the exit.
    int score;                           // Player's score.
    int moveCounter;                     // Move counter for enemy update (resets per enemy move).
    int totalMoves;                      // Persistent counter for total moves made.
    int enemyDelay;                      // Delay between enemy moves (set to 1 in our game).
    int level;                           // Current level (e.g., 1 or 2).
    bool gameOver;                       // Flag to indicate game over.

    Game();
};

// Function prototypes for game functionality.
void saveGame(const Game &game, const std::string &filename);
bool loadGame(Game &game, const std::string &filename);
Position calculateEnemyMove(const Position &enemyPos, const Position &playerPos,
                              const std::vector<std::vector<char>> &grid);
void carveGuaranteedPath(Game &game);
void initLevel(Game &game, int level);
void printGrid(const Game &game);
bool checkAndCollectPowerup(Game &game, const Position &pos);
void runGame();

#endif  // GAME_H

