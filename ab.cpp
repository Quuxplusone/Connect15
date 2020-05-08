
#include "ab.h"
#include "state.h"
#include <stdlib.h>
#include <utility>

double simplest_eval(const State& s)
{
    return (rand() % 2) ? 1 : -1;
}

std::pair<double, int> recursively_evaluate(LeafEvaluationFunction eval, const State& s, int depth)
{
    auto expectation_for_this_move = [&eval, &s, &depth](int move) -> double {
        State next = s;
        if (next.apply_in_place_without_drawing(move)) {
            return INT_MAX;  // this move is winning!
        }

        Color who = s.active_player();
        double sum = 0.0;
        int count = 0;
        for (int v = 1; v <= 7; ++v) {
            int weight = s.count_unseen_cards(who, v);
            assert(0 <= weight && weight <= 2);
            if (weight != 0) {
                next.draw_this_card(who, v);
                auto vm = recursively_evaluate(eval, next, depth - 1);
                sum += weight * vm.first;
                count += weight;
            }
        }
        return -sum / count;
    };

#if 0
    auto mm = s.must_respond_to_threat();
    if (mm.first) {
        int move = mm.second;
        if (move == -2) {
            return { INT_MIN, 0 };
        } else {
            // This move is forced.
            double expectation = expectation_for_this_move(move);
            return  { expectation, move };
        }
    }
#endif

    if ((depth == 0) || s.is_tie_game()) {
        return { eval(s), 0 };
    }

    int columns = s.count_columns();

    std::pair<double, int> best = { INT_MIN, 0 };

    if (columns == 0) {
        columns = -1;  // there's only one legal move
    } else if (columns == 1) {
        columns = 0;  // the two sides are symmetric
    }

    for (int move = -1; move <= columns; ++move) {
        double expectation = expectation_for_this_move(move);
        if (expectation == double(INT_MAX)) {
            return { INT_MAX, move };  // the best possible outcome
        }
        if (expectation > best.first) {
            best = { expectation, move };
        }
    }
    return best;
}
