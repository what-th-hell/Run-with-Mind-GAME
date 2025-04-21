#include "Game.h"
#include "Utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <cmath>

#ifdef _WIN32
    #include <windows.h>
#endif

// Game constructor.
Game::Game() : player(1, 1), score(0), moveCounter(0), totalMoves(0),
               enemyDelay(1), level(1), gameOver(false) {}

// Save game state to a file.
void saveGame(const Game &game, const std::string &filename) {
    std::ofstream out(filename);
    if (!out) {
        std::cout << "Error opening file for saving." << std::endl;
        return;
    }
    out << game.level << " " << game.score << " " << game.moveCounter << " "
        << game.totalMoves << " " << game.enemyDelay << " " << game.gameOver << "\n";
    int rows = game.grid.size();
    int cols = (rows > 0) ? game.grid[0].size() : 0;
    out << rows << " " << cols << "\n";
    for (const auto &row : game.grid) {
        for (char c : row)
            out << c;
        out << "\n";
    }
    out << game.player.pos.x << " " << game.player.pos.y << "\n";
    out << game.exitPos.x << " " << game.exitPos.y << "\n";
    out << game.enemies.size() << "\n";
    for (const auto &enemy : game.enemies)
        out << enemy.pos.x << " " << enemy.pos.y << "\n";
    out << game.powerups.size() << "\n";
    for (const auto &p : game.powerups)
        out << p.x << " " << p.y << "\n";
    out.close();
    std::cout << "Game saved to " << filename << std::endl;
}

// Load game state from a file.
bool loadGame(Game &game, const std::string &filename) {
    std::ifstream in(filename);
    if (!in) {
        std::cout << "Error opening file for loading." << std::endl;
        return false;
    }
    in >> game.level >> game.score >> game.moveCounter >> game.totalMoves >> game.enemyDelay;
    int gameOverInt;
    in >> gameOverInt;
    game.gameOver = (gameOverInt != 0);

    int rows, cols;
    in >> rows >> cols;
    game.grid.assign(rows, std::vector<char>(cols, ' '));
    std::string line;
    getline(in, line); // consume newline.
    for (int i = 0; i < rows; i++) {
        getline(in, line);
        for (int j = 0; j < cols && j < static_cast<int>(line.size()); j++)
            game.grid[i][j] = line[j];
    }
    in >> game.player.pos.x >> game.player.pos.y;
    in >> game.exitPos.x >> game.exitPos.y;

    size_t enemyCount;
    in >> enemyCount;
    game.enemies.clear();
    for (size_t i = 0; i < enemyCount; i++) {
        int ex, ey;
        in >> ex >> ey;
        game.enemies.push_back(Enemy(ex, ey));
    }

    size_t powerupCount;
    in >> powerupCount;
    game.powerups.clear();
    for (size_t i = 0; i < powerupCount; i++) {
        int px, py;
        in >> px >> py;
        game.powerups.push_back({px, py});
    }

    in.close();
    std::cout << "Game loaded from " << filename << std::endl;
    return true;
}

// Enemy chase logic: choose a step toward the player.
Position calculateEnemyMove(const Position &enemyPos, const Position &playerPos,
                              const std::vector<std::vector<char>> &grid) {
    Position next = enemyPos;
    int dx = playerPos.x - enemyPos.x;
    int dy = playerPos.y - enemyPos.y;

    if (std::abs(dx) >= std::abs(dy)) {
        if (dx > 0)
            next.x++;
        else if (dx < 0)
            next.x--;
    } else {
        if (dy > 0)
            next.y++;
        else if (dy < 0)
            next.y--;
    }

    // If the chosen move is blocked, try the other axis.
    if (!isValidMove(next, grid)) {
        next = enemyPos;
        if (dy != 0) {
            if (dy > 0)
                next.y++;
            else
                next.y--;
        }
    }
    if (!isValidMove(next, grid))
        next = enemyPos;

    return next;
}

// Carve a guaranteed L‑shaped corridor from start to exit so the maze is always solvable.
void carveGuaranteedPath(Game &game) {
    // Carve a horizontal corridor on row 1 from column 1 to COLS-2.
    for (int j = 1; j <= COLS - 2; j++) {
        game.grid[1][j] = ' ';
    }
    // Carve a vertical corridor in column COLS-2 from row 1 to ROWS-2.
    for (int i = 1; i <= ROWS - 2; i++) {
        game.grid[i][COLS - 2] = ' ';
    }
    // Ensure the starting cell and exit cell are set correctly.
    game.grid[1][1] = ' ';
    game.grid[ROWS - 2][COLS - 2] = 'E';
}

// Initialize the maze for a given level.
void initLevel(Game &game, int level) {
    game.level = level;
    game.score = 0;
    game.moveCounter = 0;
    game.totalMoves = 0;  // Reset move count at level start.
    game.gameOver = false;
    game.enemyDelay = 1;  // Enemies move after every player move.

    // Generate an empty 20x20 maze.
    game.grid.assign(ROWS, std::vector<char>(COLS, ' '));

    // Set border walls.
    for (int i = 0; i < ROWS; i++) {
        game.grid[i][0] = '#';
        game.grid[i][COLS - 1] = '#';
    }
    for (int j = 0; j < COLS; j++) {
        game.grid[0][j] = '#';
        game.grid[ROWS - 1][j] = '#';
    }

    // Use a fill chance: Level 1 has 15% and Level 2 has 25%.
    int fillChance = (level == 1) ? 15 : 25;
    for (int i = 1; i < ROWS - 1; i++) {
        for (int j = 1; j < COLS - 1; j++) {
            if ((rand() % 100) < fillChance)
                game.grid[i][j] = (rand() % 2 == 0) ? '#' : '@';
            else
                game.grid[i][j] = ' ';
        }
    }

    // For Level 2, overlay extra deterministic structures.
    if (level == 2) {
        // Create a vertical wall down the middle with gaps.
        int midCol = COLS / 2;
        for (int i = 1; i < ROWS - 1; i++) {
            if (i == ROWS / 3 || i == (2 * ROWS) / 3)
                continue;
            game.grid[i][midCol] = '#';
        }
        // Create a horizontal wall across the middle with a gap.
        int midRow = ROWS / 2;
        for (int j = 1; j < COLS - 1; j++) {
            if (j == COLS / 4)
                continue;
            game.grid[midRow][j] = '@';
        }
    }

    // Set and clear the player's starting cell.
    game.player.pos = {1, 1};
    game.grid[1][1] = ' ';

    // Define the exit cell.
    game.exitPos = {ROWS - 2, COLS - 2};
    game.grid[ROWS - 2][COLS - 2] = 'E';

    // Set up enemy positions.
    game.enemies.clear();
    if (level == 1) {
        game.enemies.push_back(Enemy(1, COLS - 2));         // Top-right.
        game.enemies.push_back(Enemy(ROWS / 2, 1));           // Middle-left.
        game.enemies.push_back(Enemy(ROWS / 2, COLS - 3));      // Middle-right.
        game.grid[1][COLS - 2] = ' ';
        game.grid[ROWS / 2][1] = ' ';
        game.grid[ROWS / 2][COLS - 3] = ' ';
    } else {
        game.enemies.push_back(Enemy(1, COLS - 2));           // Top-right.
        game.enemies.push_back(Enemy(ROWS - 2, 1));             // Bottom-left.
        game.enemies.push_back(Enemy(ROWS / 2, COLS - 2));        // Middle-right.
        game.enemies.push_back(Enemy(ROWS - 2, COLS / 2));        // Bottom-middle.
        game.enemies.push_back(Enemy(ROWS / 3, COLS / 3));        // Upper-left-ish.
        game.grid[1][COLS - 2] = ' ';
        game.grid[ROWS - 2][1] = ' ';
        game.grid[ROWS / 2][COLS - 2] = ' ';
        game.grid[ROWS - 2][COLS / 2] = ' ';
        game.grid[ROWS / 3][COLS / 3] = ' ';
    }

    // Place powerups.
    game.powerups.clear();
    if (level == 1) {
        game.powerups.push_back({ROWS / 2, COLS / 2});
        game.powerups.push_back({3, COLS - 4});
        game.grid[ROWS / 2][COLS / 2] = ' ';
        game.grid[3][COLS - 4] = ' ';
    } else {
        game.powerups.push_back({ROWS / 2, 2});
        game.powerups.push_back({ROWS - 3, COLS - 3});
        game.powerups.push_back({2, 2});
        game.grid[ROWS / 2][2] = ' ';
        game.grid[ROWS - 3][COLS - 3] = ' ';
        game.grid[2][2] = ' ';
    }

    // Guarantee a valid path from the start to the exit.
    carveGuaranteedPath(game);
}

// Check and collect a powerup if the player's position matches its position.
bool checkAndCollectPowerup(Game &game, const Position &pos) {
    for (auto it = game.powerups.begin(); it != game.powerups.end(); ++it) {
        if (it->x == pos.x && it->y == pos.y) {
            game.powerups.erase(it);
            return true;
        }
    }
    return false;
}

// Render the maze, along with the title and game statistics.
void printGrid(const Game &game) {
    std::cout << YELLOW << "=====================================" << RESET << std::endl;
    std::cout << YELLOW << "\tRun with Mind" << RESET << std::endl;
    std::cout << YELLOW << "=====================================" << RESET << std::endl;

    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (game.player.pos.x == i && game.player.pos.y == j) {
                std::cout << GREEN << game.player.getSymbol() << RESET;
                continue;
            }
            bool enemyPrinted = false;
            for (auto &enemy : game.enemies) {
                if (enemy.pos.x == i && enemy.pos.y == j) {
                    std::cout << RED << enemy.getSymbol() << RESET;
                    enemyPrinted = true;
                    break;
                }
            }
            if (enemyPrinted)
                continue;
            bool powerupPrinted = false;
            for (auto &p : game.powerups) {
                if (p.x == i && p.y == j) {
                    std::cout << YELLOW << "*" << RESET;
                    powerupPrinted = true;
                    break;
                }
            }
            if (powerupPrinted)
                continue;

            char cell = game.grid[i][j];
            if (cell == ' ') {
                if ((i + j) % 2 == 0)
                    std::cout << BG_WHITE << " " << RESET;
                else
                    std::cout << BG_GRAY << " " << RESET;
            } else if (cell == '#' || cell == '@') {
                std::cout << BLUE << cell << RESET;
            } else if (cell == 'E') {
                std::cout << MAGENTA << cell << RESET;
            } else {
                std::cout << cell;
            }
        }
        std::cout << std::endl;
    }
    std::cout << "Score: " << game.score << "   Level: " << game.level
              << "   Moves: " << game.totalMoves << std::endl;
    std::cout << "Controls: Move with WASD. Press 'M' for menu (save/load)." << std::endl;
}

// Run a single game session.
void runGame() {
    int currentLevel = 1;
    Game game;
    initLevel(game, currentLevel);

    while (true) {
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif
        printGrid(game);

        if (game.player.pos.x == game.exitPos.x && game.player.pos.y == game.exitPos.y) {
            if (game.level == 1) {
                std::cout << "Level 1 Complete! Proceeding to Level 2..." << std::endl;
#ifdef _WIN32
                system("pause");
#else
                std::cout << "Press any key to continue...";
                getInputChar();
#endif
                currentLevel = 2;
                initLevel(game, currentLevel);
                continue;
            } else {
                std::cout << "Congratulations! You completed Level 2 and won the game!" << std::endl;
                break;
            }
        }

        char key = getInputChar();
        if (key == 'm' || key == 'M') {
            std::cout << "\nEnter command (save/load): ";
            std::string command;
            std::cin >> command;
            if (command == "save") {
                saveGame(game, "savegame.txt");
                std::cout << "Press any key to continue...";
                getInputChar();
            } else if (command == "load") {
                if (loadGame(game, "savegame.txt"))
                    std::cout << "Press any key to continue...";
                getInputChar();
            }
            continue;
        }

        Position newPos = game.player.pos;
        if (key == 'W' || key == 'w')
            newPos.x--;
        else if (key == 'S' || key == 's')
            newPos.x++;
        else if (key == 'A' || key == 'a')
            newPos.y--;
        else if (key == 'D' || key == 'd')
            newPos.y++;
        else
            continue;

        if (isValidMove(newPos, game.grid)) {
            game.player.pos = newPos;
            game.moveCounter++;
            game.totalMoves++;  // Increment overall moves counter.

            if (checkAndCollectPowerup(game, newPos)) {
                game.score += 10;
                std::cout << "Powerup collected! Score increased." << std::endl;
#ifdef _WIN32
                system("pause");
#else
                std::cout << "Press any key to continue...";
                getInputChar();
#endif
            }
        }

        // Enemies move after every valid move.
        if (game.moveCounter >= game.enemyDelay) {
            if (game.level == 1) {
                for (auto &enemy : game.enemies) {
                    enemy.pos = calculateEnemyMove(enemy.pos, game.player.pos, game.grid);
                }
            } else {
                std::vector<Position> newEnemyPositions;
                for (auto &enemy : game.enemies) {
                    Position next = calculateEnemyMove(enemy.pos, game.player.pos, game.grid);
                    newEnemyPositions.push_back(next);
                }
                for (size_t i = 0; i < game.enemies.size(); i++) {
                    game.enemies[i].pos = newEnemyPositions[i];
                }
            }
            game.moveCounter = 0;
        }

        // Check for collisions with enemies.
        for (auto &enemy : game.enemies) {
            if (enemy.pos.x == game.player.pos.x && enemy.pos.y == game.player.pos.y) {
                std::cout << "An enemy has caught you! Game Over." << std::endl;
                game.gameOver = true;
                break;
            }
        }
        if (game.gameOver)
            break;
    }

    std::cout << "Final Score: " << game.score << std::endl;
    std::cout << "Total Moves Made: " << game.totalMoves << std::endl;
}

