#ifndef ENTITY_H
#define ENTITY_H

// Structure for representing a grid coordinate.
struct Position {
    int x, y;
};

// The following function prototype can be used if you wish to check bounds
// in a generic way. (Its definition is provided in Game.cpp.)
bool inBounds(const Position &pos, int rows, int cols);

// Base class for any game entity.
class Entity {
public:
    Position pos;
    Entity(int x, int y);
    virtual char getSymbol() const;
    virtual ~Entity();
};

// Derived Player class.
class Player : public Entity {
public:
    Player(int x, int y);
    virtual char getSymbol() const override;
};

// Derived Enemy class.
class Enemy : public Entity {
public:
    Enemy(int x, int y);
    virtual char getSymbol() const override;
};

#endif  // ENTITY_H

