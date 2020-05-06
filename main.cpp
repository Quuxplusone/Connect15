
#include "connect15.h"
#include "ab-timed.h"
#include <iostream>
#include <stdlib.h>
#include <time.h>

#define LOOP_FOREVER 1
#define ALL_AI_PLAYERS 1

int main()
{
    srand(time(nullptr));

#if LOOP_FOREVER
    while (true) {
#endif

    State s = State::initial(rand);

    for (Color who = Red; true; who = Color(1 - who)) {
        std::string swho = ((who == Red) ? "Red" : "Black");
        auto vm = recursively_evaluate(simplest_eval, s, std::chrono::milliseconds(300));
        std::cout << s.toString() << "\n";
        std::cout << "AI thinks " << swho << "'s best move is " << vm.second << " (value " << vm.first << ").\n";
        std::cout << "AI scheduled " << recursively_scheduled_tasks << " tasks and ran " << recursively_evaluated_tasks << ".\n";
        std::cout << swho << "'s move? " << std::flush;
#if ALL_AI_PLAYERS
        std::string smove = "a";
#else
        std::string smove;
        std::cin >> smove;
        assert(bool(std::cin));  // no error handling yet
#endif
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

#if LOOP_FOREVER
    }
#endif
}
