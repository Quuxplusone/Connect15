
#include "connect15.h"
#include <iostream>

int main()
{
    State s = State::initial(rand);

    for (Color who = Red; true; who = Color(1 - who)) {
        std::cout << s.toString() << "\n";
        std::cout << ((who == Red) ? "Red" : "Black") << "'s move? " << std::flush;
        int column = 0;
        std::cin >> column;
        assert(bool(std::cin));  // no error handling yet
        bool won = s.apply_in_place(rand, column);
        if (won) {
            std::cout << "You just won the game!\n";
            break;
        }
        if (s.is_tie_game()) {
            std::cout << "Tie game!\n";
            break;
        }
    }
}
