#include "Game.h"
#include "Utils.h"
#include <cstdlib>
#include <ctime>
#include <iostream>

int main() {
    srand(static_cast<unsigned int>(time(NULL)));
    char choice;
    do {
        runGame();
        std::cout << "Play Again? (Y/N): ";
        std::cin >> choice;
        std::cin.ignore();
    } while (choice == 'Y' || choice == 'y');

    std::cout << "Thank you for playing!" << std::endl;
    return 0;
}

