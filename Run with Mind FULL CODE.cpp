#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>

using namespace std;

// ANSI color codes.
const string RESET   = "\033[0m";
const string RED     = "\033[1;31m";
const string GREEN   = "\033[1;32m";
const string YELLOW  = "\033[1;33m";
const string BLUE    = "\033[1;34m";
const string MAGENTA = "\033[1;35m";

// Background colors.
const string BG_WHITE = "\033[47m";
const string BG_GRAY  = "\033[100m";

// Maze dimensions: 20x20.
const int ROWS = 20;
const int COLS = 20;

// Simple structure for coordinates.
struct Position {
    int x, y;
};

// Check if a position is within bounds.
bool inBounds(const Position &pos) {
    return pos.x >= 0 && pos.x < ROWS && pos.y >= 0 && pos.y < COLS;
}

// Check if a move is valid (i.e. not into a wall).
bool isValidMove(const Position &pos, const vector<vector<char>> &grid) {
    if (!inBounds(pos))
        return false;
    // Both wall types ('#' and '@') block movement.
    if (grid[pos.x][pos.y] == '#' || grid[pos.x][pos.y] == '@')
        return false;
    return true;
}

// ---------------------
// Immediate Input Helper: getInputChar()
// ---------------------
#ifdef _WIN32
    #include <conio.h>
    char getInputChar() {
        return _getch();
    }
#else
    #include <unistd.h>
    #include <termios.h>
    char getInputChar() {
        char buf = 0;
        struct termios old = {0};
        if (tcgetattr(0, &old) < 0)
            perror("tcgetattr()");
        old.c_lflag &= ~ICANON;
        old.c_lflag &= ~ECHO;
        old.c_cc[VMIN] = 1;
        old.c_cc[VTIME] = 0;
        if (tcsetattr(0, TCSANOW, &old) < 0)
            perror("tcsetattr ICANON");
        if (read(0, &buf, 1) < 0)
            perror("read()");
        old.c_lflag |= ICANON;
        old.c_lflag |= ECHO;
        if (tcsetattr(0, TCSADRAIN, &old) < 0)
            perror ("tcsetattr ~ICANON");
        return buf;
    }
#endif

// ---------------------
// Game Entities
// ---------------------

// Base class for game entities.
class Entity {
public:
    Position pos;
    Entity(int x, int y) : pos({x, y}) {}
    virtual char getSymbol() const { return '?'; }
    virtual ~Entity() {}
};

// Player class.
class Player : public Entity {
public:
    Player(int x, int y) : Entity(x, y) {}
    virtual char getSymbol() const override { return 'P'; }
};

// Enemy class.
class Enemy : public Entity {
public:
    Enemy(int x, int y) : Entity(x, y) {}
    virtual char getSymbol() const override { return 'X'; }
};

// ---------------------
// Game State Structure
// ---------------------
struct Game {
    vector<vector<char>> grid; // The maze grid (walls, exit, etc.)
    Player player;             // The player
    vector<Enemy> enemies;     // Enemies in the game
    vector<Position> powerups; // Collectible powerups
    Position exitPos;          // Exit cell
    int score;                 // Player's score
    int moveCounter;           // Counts moves for enemy update (resets per enemy move)
    int totalMoves;            // Total moves made by player (persistent counter)
    int enemyDelay;            // Delay between enemy moves (set to 1 so enemies move after every move)
    int level;                 // Current level (1 or 2)
    bool gameOver;             // Game over flag

    Game() : player(1, 1), score(0), moveCounter(0), totalMoves(0),
             enemyDelay(1), level(1), gameOver(false) {}
};

// ---------------------
// Enemy Chase Logic
// ---------------------
Position calculateEnemyMove(const Position &enemyPos, const Position &playerPos,
                            const vector<vector<char>> &grid) {
    Position next = enemyPos;
    int dx = playerPos.x - enemyPos.x;
    int dy = playerPos.y - enemyPos.y;

    if (abs(dx) >= abs(dy)) {
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

    // If the chosen move is blocked, try the alternative axis.
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

// ---------------------
// Save/Load Functions
// ---------------------
void saveGame(const Game &game, const string &filename) {
    ofstream out(filename);
    if (!out) {
        cout << "Error opening file for saving." << endl;
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
    cout << "Game saved to " << filename << endl;
}

bool loadGame(Game &game, const string &filename) {
    ifstream in(filename);
    if (!in) {
        cout << "Error opening file for loading." << endl;
        return false;
    }
    in >> game.level >> game.score >> game.moveCounter >> game.totalMoves
       >> game.enemyDelay;
    int gameOverInt;
    in >> gameOverInt;
    game.gameOver = (gameOverInt != 0);

    int rows, cols;
    in >> rows >> cols;
    game.grid.assign(rows, vector<char>(cols, ' '));
    string line;
    getline(in, line); // consume newline.
    for (int i = 0; i < rows; i++) {
        getline(in, line);
        for (int j = 0; j < cols && j < (int)line.size(); j++)
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
    cout << "Game loaded from " << filename << endl;
    return true;
}

// ---------------------
// Guaranteed Path: Carve a Corridor from Start to Exit
// ---------------------
void carveGuaranteedPath(Game &game) {
    // Carve a horizontal corridor on row 1 from col 1 to col (COLS - 2)
    for (int j = 1; j <= COLS - 2; j++) {
        game.grid[1][j] = ' ';
    }
    // Carve a vertical corridor in column (COLS - 2) from row 1 to row (ROWS - 2)
    for (int i = 1; i <= ROWS - 2; i++) {
        game.grid[i][COLS - 2] = ' ';
    }
    // Ensure the start and exit cells are correct.
    game.grid[1][1] = ' ';
    game.grid[ROWS - 2][COLS - 2] = 'E';
}

// ---------------------
// Maze Generation & Level Initialization
// ---------------------
void initLevel(Game &game, int level) {
    game.level = level;
    game.score = 0;
    game.moveCounter = 0;
    game.totalMoves = 0;  // Reset total moves at start of level
    game.gameOver = false;
    game.enemyDelay = 1; // Enemies move after every move.

    // Generate the maze grid.
    game.grid.assign(ROWS, vector<char>(COLS, ' '));

    // Set border walls.
    for (int i = 0; i < ROWS; i++) {
        game.grid[i][0] = '#';
        game.grid[i][COLS - 1] = '#';
    }
    for (int j = 0; j < COLS; j++) {
        game.grid[0][j] = '#';
        game.grid[ROWS - 1][j] = '#';
    }

    // Use a different wall fill chance per level.
    int fillChance = (level == 1) ? 15 : 25;

    // Fill the interior randomly.
    for (int i = 1; i < ROWS - 1; i++) {
        for (int j = 1; j < COLS - 1; j++) {
            if ((rand() % 100) < fillChance) {
                game.grid[i][j] = (rand() % 2 == 0) ? '#' : '@';
            } else {
                game.grid[i][j] = ' ';
            }
        }
    }

    // For Level 2, overlay additional deterministic maze structures.
    if(level == 2) {
        // Create a vertical wall down the middle with gaps.
        int midCol = COLS / 2;
        for (int i = 1; i < ROWS - 1; i++) {
            if (i == ROWS/3 || i == (2*ROWS)/3)
                continue;
            game.grid[i][midCol] = '#';
        }
        // Create a horizontal wall across the middle with a gap.
        int midRow = ROWS / 2;
        for (int j = 1; j < COLS - 1; j++) {
            if (j == COLS/4)
                continue;
            game.grid[midRow][j] = '@';
        }
    }

    // Set player's starting cell and clear it.
    game.player.pos = {1, 1};
    game.grid[1][1] = ' ';

    // Define the exit cell and mark it.
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

    // Ensure a valid path exists.
    carveGuaranteedPath(game);
}

// ---------------------
// Powerup Collection Check
// ---------------------
bool checkAndCollectPowerup(Game &game, const Position &pos) {
    for (auto it = game.powerups.begin(); it != game.powerups.end(); ++it) {
        if (it->x == pos.x && it->y == pos.y) {
            game.powerups.erase(it);
            return true;
        }
    }
    return false;
}

// ---------------------
// Rendering the Maze with Colors and Title
// ---------------------
void printGrid(const Game &game) {
    // Print the game title.
    cout << YELLOW << "=====================================" << RESET << endl;
    cout << YELLOW << "\tRun with Mind" << RESET << endl;
    cout << YELLOW << "=====================================" << RESET << endl;

    // Draw the maze grid.
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            // Player takes highest priority.
            if (game.player.pos.x == i && game.player.pos.y == j) {
                cout << GREEN << game.player.getSymbol() << RESET;
                continue;
            }
            bool enemyPrinted = false;
            for (const auto &enemy : game.enemies) {
                if (enemy.pos.x == i && enemy.pos.y == j) {
                    cout << RED << enemy.getSymbol() << RESET;
                    enemyPrinted = true;
                    break;
                }
            }
            if (enemyPrinted)
                continue;
            bool powerupPrinted = false;
            for (const auto &p : game.powerups) {
                if (p.x == i && p.y == j) {
                    cout << YELLOW << "*" << RESET;
                    powerupPrinted = true;
                    break;
                }
            }
            if (powerupPrinted)
                continue;

            char cell = game.grid[i][j];
            if (cell == ' ') {
                if ((i + j) % 2 == 0)
                    cout << BG_WHITE << " " << RESET;
                else
                    cout << BG_GRAY << " " << RESET;
            } else if (cell == '#' || cell == '@') {
                cout << BLUE << cell << RESET;
            } else if (cell == 'E') {
                cout << MAGENTA << cell << RESET;
            } else {
                cout << cell;
            }
        }
        cout << endl;
    }
    // Display game stats: score, level, and total moves made.
    cout << "Score: " << game.score << "   Level: " << game.level
         << "   Moves: " << game.totalMoves << endl;
    cout << "Controls: Move with WASD. Press 'M' for menu (save/load)." << endl;
}

// ---------------------
// Main Game Session
// ---------------------
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

        // Check win condition: reaching the exit.
        if (game.player.pos.x == game.exitPos.x && game.player.pos.y == game.exitPos.y) {
            if (game.level == 1) {
                cout << "Level 1 Complete! Proceeding to Level 2..." << endl;
#ifdef _WIN32
                system("pause");
#else
                cout << "Press any key to continue...";
                getInputChar();
#endif
                currentLevel = 2;
                initLevel(game, currentLevel);
                continue;
            } else {
                cout << "Congratulations! You completed Level 2 and won the game!" << endl;
                break;
            }
        }

        // Get immediate key input.
        char key = getInputChar();
        if (key == 'm' || key == 'M') {
            cout << "\nEnter command (save/load): ";
            string command;
            cin >> command;
            if (command == "save") {
                saveGame(game, "savegame.txt");
                cout << "Press any key to continue...";
                getInputChar();
            } else if (command == "load") {
                if (loadGame(game, "savegame.txt"))
                    cout << "Press any key to continue...";
                getInputChar();
            }
            continue;
        }

        // Process player movement.
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
            game.totalMoves++;  // Increment the overall moves counter.

            if (checkAndCollectPowerup(game, newPos)) {
                game.score += 10;
                cout << "Powerup collected! Score increased." << endl;
#ifdef _WIN32
                system("pause");
#else
                cout << "Press any key to continue...";
                getInputChar();
#endif
            }
        }

        // Enemies move after every valid player move.
        if (game.moveCounter >= game.enemyDelay) {
            if (game.level == 1) {
                for (auto &enemy : game.enemies) {
                    enemy.pos = calculateEnemyMove(enemy.pos, game.player.pos, game.grid);
                }
            } else {
                vector<Position> newEnemyPositions;
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

        // Check collision: if an enemy catches the player.
        for (auto &enemy : game.enemies) {
            if (enemy.pos.x == game.player.pos.x && enemy.pos.y == game.player.pos.y) {
                cout << "An enemy has caught you! Game Over." << endl;
                game.gameOver = true;
                break;
            }
        }
        if (game.gameOver)
            break;
    }

    cout << "Final Score: " << game.score << endl;
    cout << "Total Moves Made: " << game.totalMoves << endl;
}

// ---------------------
// Main: Play Again Menu
// ---------------------
int main() {
    srand(time(NULL));
    char choice;
    do {
        runGame();
        cout << "Play Again? (Y/N): ";
        cin >> choice;
        cin.ignore();
    } while(choice == 'Y' || choice == 'y');

    cout << "Thank you for playing!" << endl;
    return 0;
}
