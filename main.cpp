
#include "connect15.h"
#include "ab.h"
#include <iostream>
#include <stdlib.h>
#include <time.h>

int main()
{
    srand(time(nullptr));

    State s = State::initial(rand);

    for (Color who = Red; true; who = Color(1 - who)) {
        std::string swho = ((who == Red) ? "Red" : "Black");
        std::cout << s.toString() << "\n";
        auto vm = recursively_evaluate(simplest_eval, s, 4);
        std::cout << "AI thinks " << swho << "'s best move is " << vm.second << " (value " << vm.first << ").\n";
        std::cout << swho << "'s move? " << std::flush;
        std::string smove;
        std::cin >> smove;
        assert(bool(std::cin));  // no error handling yet
        int move = -2;
        if (smove == "A" || smove == "a") {
            move = vm.second;
        } else {
            move = atoi(smove.c_str());
        }
        bool won = s.apply_in_place(rand, move);
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
