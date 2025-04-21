#include "Entity.h"

// Check if the position is within the bounds given rows and columns.
bool inBounds(const Position &pos, int rows, int cols) {
    return pos.x >= 0 && pos.x < rows && pos.y >= 0 && pos.y < cols;
}

// Base class definitions
Entity::Entity(int x, int y) : pos({x, y}) {}

char Entity::getSymbol() const {
    return '?';
}

Entity::~Entity() {}

// Player
Player::Player(int x, int y) : Entity(x, y) {}

char Player::getSymbol() const {
    return 'P';
}

// Enemy
Enemy::Enemy(int x, int y) : Entity(x, y) {}

char Enemy::getSymbol() const {
    return 'X';
}

