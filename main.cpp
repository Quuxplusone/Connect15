
#include "connect15.h"
#include "ab-timed.h"
#include <iostream>
#include <stdlib.h>
#include <time.h>

#define LOOP_FOREVER 1
#define ALL_AI_PLAYERS 1

int wins[3] = { 0, 0, 0 };

int main()
{
    srand(time(nullptr));

#if LOOP_FOREVER
    while (true) {
#endif

    State s = State::initial(rand);

    for (Color who = Red; true; who = Color(1 - who)) {
        std::string swho = ((who == Red) ? "Red" : "Black");
        auto vm = recursively_evaluate(simplest_eval, s, std::chrono::milliseconds(150));
        std::cout << s.toString() << "\n";
        std::cout << "AI thinks " << swho << "'s best move is " << vm.second << " (value " << vm.first << ").\n";
        printf("Scheduled %d tasks, ran %d tasks, search depth %d.\n",
               recursively_scheduled_tasks, recursively_evaluated_tasks, max_search_depth.load());
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
            std::cout << swho << " just won the game!\n";
            wins[who] += 1;
            break;
        }
        if (s.is_tie_game()) {
            std::cout << "Tie game!\n";
            wins[Nobody] += 1;
            break;
        }
    }

    printf("%*s Wins: Red %d, Black %d, Nobody %d\n", 40, "", wins[Red], wins[Black], wins[Nobody]);

#if LOOP_FOREVER
    }
#endif
}
