#include "ab-timed.h"
#include "matchbox_player.h"
#include "state.h"
#include <functional>
#include <iostream>
#include <random>
#include <stdlib.h>
#include <time.h>

int wins[2][3] = {};

int main(int argc, char **argv)
{
    const bool play_versus_human = (argc > 1 && argv[1] == std::string("me"));
    const bool train_versus_ai = !play_versus_human;
    int seed = (argc > 1) ? atoi(argv[1]) : 0;

    std::mt19937 true_rand;
    true_rand.seed(time(nullptr));

    MatchboxPlayer mp;
    mp.load_from_file("matchboxes.dat");

restart:
    std::mt19937 reproducible_rand;
    reproducible_rand.seed(seed ? seed : time(nullptr));

    memset(wins, '\0', sizeof(wins));

    for (size_t games_played = 0; true; ++games_played) {
        State s = State::initial(std::ref(reproducible_rand));
        Color mpColor = Color(games_played % 2);
        Color humanColor = play_versus_human ? Color(1 - mpColor) : Nobody;

        auto get_human_move = [&](const char *swho) {
            std::cout << swho << "'s move? " << std::flush;
            int move;
            std::cin >> move;
            assert(std::cin.good());
            assert(-1 <= move && move <= s.count_columns());
            return move;
        };

        auto get_bfs_move = [&](const char *swho) {
            auto vm = recursively_evaluate(simplest_eval, s, std::chrono::milliseconds(10));
            std::cout << "AI thinks " << swho << "'s best move is " << vm.second << " (value " << vm.first << ").\n";
            printf("Scheduled %d tasks, ran %d tasks, search depth %d.\n",
                   recursively_scheduled_tasks, recursively_evaluated_tasks, max_search_depth.load());
            if (vm.first >= INT_MAX) {
                mp.record_definitely_best_move(s, vm.second);
            }
            return vm.second;
        };

        auto get_mp_move = [&](const char *swho) {
            auto vm = recursively_evaluate(simplest_eval, s, std::chrono::milliseconds(50));
            if (vm.first >= INT_MAX) {
                std::cout << "AI sees the winning move " << vm.second << " and is forcing MP to take it.\n";
                mp.record_definitely_best_move(s, vm.second);
            }
            auto pm = mp.pick_move(std::ref(true_rand), s);
            std::cout << "MP picked " << pm.move << " for " << swho << ".";
            if (pm.was_familiar) {
                std::cout << "                 This position was familiar!";
            }
            std::cout << "\n";
            return pm.move;
        };

        for (Color who = Red; true; who = Color(1 - who)) {

            auto fm = s.must_respond_to_threat();
            if (fm.is_forced) {
                mp.record_definitely_best_move(s, fm.move);
            }

            std::cout << s.toString() << "\n";
            const char *swho = ((who == Red) ? "Red" : "Black");
            int move;
            if (who == mpColor) {
                move = get_mp_move(swho);
            } else if (who == humanColor) {
                move = get_human_move(swho);
            } else {
                move = get_bfs_move(swho);
            }
            bool won = s.apply_in_place(std::ref(reproducible_rand), move);
            if (won) {
                std::cout << swho << " just won the game!\n";
                wins[mpColor][who] += 1;
                if (who == mpColor) {
                    mp.record_win_and_reset();
                } else {
                    mp.record_loss_and_reset();
                }
                break;
            } else if (s.is_tie_game()) {
                std::cout << "Tie game!\n";
                wins[mpColor][Nobody] += 1;
                mp.record_tie_and_reset();
                break;
            }
        }

        if (train_versus_ai) {
            printf("%*s Wins when MP plays red: R %d, B %d, Tie %d\n", 40, "", wins[0][Red], wins[0][Black], wins[0][Nobody]);
            printf("%*s                  black: R %d, B %d, Tie %d\n", 40, "", wins[1][Red], wins[1][Black], wins[1][Nobody]);
            if (games_played % 16 == 0) {
                mp.save_to_file("matchboxes.dat");
            }
            if (seed != 0 && games_played == 31) goto restart;
        }
    }
}
