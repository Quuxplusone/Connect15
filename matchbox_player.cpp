
#include "matchbox_player.h"

#include <stdint.h>
#include <stdio.h>
#include <utility>

template<class Pair>
static bool read_from_file(FILE *fp, Pair& kv)
{
    size_t nbytes = fread(kv.first.data_, 1, 32, fp);
    if (nbytes != 32) {
        assert(nbytes == 0);
        return false;
    }
    uint8_t num_matchboxes = 0;
    nbytes = fread(&num_matchboxes, 1, 1, fp);
    assert(nbytes == 1);
    assert(1 <= num_matchboxes && num_matchboxes <= 28);
    memset(kv.second.weights_, '\0', 28);
    nbytes = fread(kv.second.weights_, 1, num_matchboxes, fp);
    assert(nbytes == num_matchboxes);
    return true;
}

template<class Pair>
static void write_to_file(FILE *fp, const Pair& kv)
{
    fwrite(kv.first.data_, 1, 32, fp);
    uint8_t num_matchboxes = kv.second.num_matchboxes();
    fwrite(&num_matchboxes, 1, 1, fp);
    fwrite(kv.second.weights_, 1, num_matchboxes, fp);
}

void MatchboxPlayer::load_from_file(const char *filename)
{
    if (FILE *fp = fopen(filename, "r")) {
        std::pair<PackedState, Choices> kv;
        while (read_from_file(fp, kv)) {
            map_.insert(kv);
        }
        fclose(fp);
    }
}

void MatchboxPlayer::save_to_file(const char *filename) const
{
    FILE *fp = fopen(filename, "w");
    for (const auto& kv : map_) {
        write_to_file(fp, kv);
    }
    fclose(fp);
}

void MatchboxPlayer::record_definitely_winning_move(const State& s, int move)
{
    int columns = s.count_columns();
    std::pair<PackedState, bool> key_flipHorizontal = s.toPackedCanonical();
    const PackedState& key = key_flipHorizontal.first;
    auto it = map_.find(key);
    if (it == map_.end()) {
        it = map_.emplace(key, Choices(s.count_columns() + 1)).first;
    }
    if (key_flipHorizontal.second) {
        move = s.count_columns() - move - 1;
    }
    it->second.record_definitely_winning_move(move);
}

void MatchboxPlayer::record_win_and_reset()
{
    for (const auto& cm : history_) {
        cm.first->increase_weight(cm.second);
    }
    history_.resize(0);
}

void MatchboxPlayer::record_loss_and_reset()
{
    for (const auto& cm : history_) {
        cm.first->decrease_weight(cm.second);
    }
    history_.resize(0);
}

void MatchboxPlayer::record_tie_and_reset()
{
    history_.resize(0);
}
