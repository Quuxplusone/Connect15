#pragma once

#include <stdint.h>
#include <unordered_map>
#include <utility>
#include <vector>

#include "packed_state.h"
#include "state.h"

class MatchboxPlayer {
public:
    void load_from_file(const char *filename);
    void save_to_file(const char *filename) const;

    template<class Random>
    int pick_move(Random rand, const State& s);

    void record_definitely_best_move(const State& s, int m);
    void record_win_and_reset();
    void record_loss_and_reset();
    void record_tie_and_reset();

private:
    struct Choices {
        uint8_t weights_[28];

        Choices() = default;

        explicit Choices(int n) : weights_{} {
            for (int i=0; i < n; ++i) {
                weights_[i] = 16;
            }
        }

        int num_matchboxes() const {
            for (int i=28; i > 0; --i) {
                if (weights_[i-1] != 0) return i;
            }
            return 0;
        }

        void record_definitely_best_move(int m) {
            memset(weights_, '\0', 28);
            weights_[m+1] = 16;
        }

        void halve_all_weights() {
            for (auto& w : weights_) {
                w = (w == 1) ? 1 : (w / 2);
            }
        }

        bool maybe_double_all_weights() {
            for (auto& w : weights_) {
                if (w >= 64) return false;
            }
            for (auto& w : weights_) {
                w *= 2;
            }
            return true;
        }

        void increase_weight(int i) {
            assert(weights_[i] > 0);
            weights_[i] += 1;
            if (weights_[i] >= 128) {
                halve_all_weights();
            }
        }

        void decrease_weight(int i) {
            assert(weights_[i] > 0);
            if (weights_[i] > 1 || maybe_double_all_weights()) {
                weights_[i] -= 1;
            }
        }

        template<class Random>
        int pick_move(Random rand) const {
            int sum = std::accumulate(weights_, weights_ + 28, 0);
            assert(sum >= 0);
            int count = rand() % sum;
            for (int i=0; i < 28; ++i) {
                count -= weights_[i];
                if (count < 0) return i-1;
            }
            assert(false);
        }
    };

    std::unordered_map<PackedState, Choices> map_;
    std::vector<std::pair<Choices*, int>> history_;
};

template<class Random>
int MatchboxPlayer::pick_move(Random rand, const State& s)
{
    int columns = s.count_columns();
    std::pair<PackedState, bool> key_flipHorizontal = s.toPackedCanonical();
    const PackedState& key = key_flipHorizontal.first;
    auto it = map_.find(key);
    if (it == map_.end()) {
        it = map_.emplace(key, Choices(s.count_columns() + 1)).first;
    }
    Choices& choices = it->second;
    int move = choices.pick_move(rand);
    history_.push_back({ &choices, move+1 });  // when move==-1, it affects weights_[0], and so on
    if (key_flipHorizontal.second) {
        move = s.count_columns() - move - 1;
    }
    return move;
}
