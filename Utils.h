#ifndef UTILS_H
#define UTILS_H

// Returns a single character from input without waiting for Enter.
// On Windows it uses conio.h (_getch()) while on Unix systems it sets the terminal to raw mode.
char getInputChar();

#endif  // UTILS_H

