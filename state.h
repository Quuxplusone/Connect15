#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <numeric>
#include <string>

#include "board_etc.h"
#include "nibble_writer.h"
#include "packed_state.h"

struct State {
    explicit State(Color who, Card top_red, Card top_black, Board b) :
        board_(std::move(b)),
        top_card_{top_red, top_black},
        who_(who)
    {
        board_.populate_unseen_cards(unseen_cards_);
    }

    template<class Random>
    static State initial(Random rand) {
        State s;
        for (int v=1; v <= 7; ++v) {
            s.unseen_cards_[Red][v] = 2;
            s.unseen_cards_[Black][v] = 2;
        }
        s.draw_random_card(rand, Red);
        s.draw_random_card(rand, Black);
        return s;
    }

    template<class Random>
    void draw_random_card(Random rand, Color who) {
        int sum = count_unseen_cards(who);
        if (sum == 0) {
            top_card_[who] = Card();
        } else {
            assert(0 < sum && sum <= 14);
            int k = rand() % sum;
            int v = 0;
            while (k >= 0) {
                k -= unseen_cards_[who][++v];
            }
            draw_this_card(who, v);
        }
    }

    void draw_this_card(Color who, int v) {
        assert(1 <= v && v <= 7);
        assert(unseen_cards_[who][v] >= 1);
        unseen_cards_[who][v] -= 1;
        top_card_[who] = Card(who, v);
    }

    std::string toString() const {
        std::string result = board_.toString() + "\n";
        result += "Red's top card: " + top_card_[Red].toString() + "\n";
        result += "Black's top card: " + top_card_[Black].toString();
        return result;
    }

    nibble_writer toPacked(nibble_writer it) const {
        it.write(who_);
        it = top_card_[Red].toPacked(it);
        it = top_card_[Black].toPacked(it);
        it = board_.toPacked(it);
        return it;
    }

    PackedState toPacked() const {
        PackedState p;
        nibble_writer it = nibble_writer(p.data_);
        it = this->toPacked(it);
        uint8_t *end = it.round_off_and_get();
        assert((end - p.data_) <= sizeof p.data_);
        return p;
    }

    Color active_player() const {
        return who_;
    }

    bool is_tie_game() const {
        return top_card_[who_].color() == Nobody;
    }

    std::pair<bool, int> must_respond_to_threat() const {
        Color whont = Color(1 - who_);
        if (top_card_[whont].color() == Nobody) {
            return { false, 0 };
        }
        return board_.must_respond_to_threat(top_card_[whont]);
    }

    bool apply_in_place_without_drawing(int column) {
        Card card = std::exchange(top_card_[who_], Card());
        board_.apply_in_place(column, card);
        who_ = Color(1 - who_);
        return board_.is_win_involving(column, card);
    }

    template<class Random>
    bool apply_in_place(Random rand, int column) {
        Card card = top_card_[who_];
        board_.apply_in_place(column, card);
        draw_random_card(rand, who_);
        who_ = Color(1 - who_);
        return board_.is_win_involving(column, card);
    }

    template<class Random>
    State apply(Random rand, int column) const {
        State next = *this;
        next.apply_in_place(rand, column);
        return next;
    }

    int count_columns() const { return board_.count_columns(); }

    int count_unseen_cards(Color who, int v) const {
        assert(1 <= v && v <= 7);
        return unseen_cards_[who][v];
    }

    int count_unseen_cards(Color who) const {
        int sum = std::accumulate(std::begin(unseen_cards_[who]), std::end(unseen_cards_[who]), 0);
        assert(0 <= sum && sum <= 14);
        return sum;
    }

private:
    explicit State() = default;

    Board board_;
    int8_t unseen_cards_[2][8] = {};
    Card top_card_[2];
    Color who_ = Red;
};
