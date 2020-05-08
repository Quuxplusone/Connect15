
#include "ab-timed.h"
#include "matchbox_player.h"
#include "state.h"
#include <iostream>
#include <stdlib.h>
#include <time.h>

#define LOOP_FOREVER 1
#define ALL_AI_PLAYERS 1

int winsByColor[3] = { 0, 0, 0 };
int winsByPlayer[3] = { 0, 0, 0 };

int main()
{
    MatchboxPlayer mp;
    mp.load_from_file("matchboxes.dat");

    srand(time(nullptr));

    for (size_t games_played = 0; true; ++games_played) {
        State s = State::initial(rand);
        Color mpColor = Color(games_played % 2);

        auto get_bfs_move = [&](const char *swho) {
            auto vm = recursively_evaluate(simplest_eval, s, std::chrono::milliseconds(50));
            std::cout << "AI thinks " << swho << "'s best move is " << vm.second << " (value " << vm.first << ").\n";
            printf("Scheduled %d tasks, ran %d tasks, search depth %d.\n",
                   recursively_scheduled_tasks, recursively_evaluated_tasks, max_search_depth.load());
            return vm.second;
        };

        auto get_mp_move = [&](const char *swho) {
            int move = mp.pick_move(rand, s);
            std::cout << "MP picked " << move << " for " << swho << ".\n";
            return move;
        };

        for (Color who = Red; true; who = Color(1 - who)) {
            std::cout << s.toString() << "\n";
            const char *swho = ((who == Red) ? "Red" : "Black");
            int move;
            if (who == mpColor) {
                move = get_mp_move(swho);
            } else {
                move = get_bfs_move(swho);
            }
            bool won = s.apply_in_place(rand, move);
            if (won) {
                std::cout << swho << " just won the game!\n";
                winsByColor[who] += 1;
                winsByPlayer[who == mpColor] += 1;
                if (who == mpColor) {
                    mp.record_win_and_reset();
                } else {
                    mp.record_loss_and_reset();
                }
                break;
            } else if (s.is_tie_game()) {
                std::cout << "Tie game!\n";
                winsByColor[Nobody] += 1;
                winsByPlayer[2] += 1;
                mp.record_tie_and_reset();
                break;
            }
        }

        printf("%*s Wins: Red %d, Black %d, Nobody %d\n", 40, "", winsByColor[Red], winsByColor[Black], winsByColor[Nobody]);
        printf("%*s       AI %d, MP %d, tie %d\n", 40, "", winsByPlayer[0], winsByPlayer[1], winsByPlayer[2]);
        if (games_played % 16 == 0) {
            mp.save_to_file("matchboxes.dat");
        }
    }
}
