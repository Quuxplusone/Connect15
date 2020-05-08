#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <numeric>
#include <string>
#include <vector>

#include "nibble_writer.h"
#include "packed_state.h"

enum Color {
    Red = 0,
    Black = 1,
    Nobody = 2,
};

struct Card {
    explicit Card() : who_(Nobody), value_(0) {}
    explicit Card(Color who, int value) : who_(who), value_(value) {}
    explicit Card(const char *s) {
        assert('1' <= s[0] && s[0] <= '7');
        assert(s[1] == 'r' || s[1] == 'b');
        assert(s[2] == '\0');
        who_ = (s[1] == 'r') ? Red : Black;
        value_ = (s[0] - '0');
    }
    Color color() const { return who_; }
    int value() const { return value_; }

    std::string toString() const {
        std::string result = "..";
        if (who_ != Nobody) {
            result[0] = ('0' + value_);
            result[1] = (who_ == Red) ? 'r' : 'b';
        }
        return result;
    }

    nibble_writer toPacked(nibble_writer it) const {
        int value = (who_ == Nobody) ? 0 : ((who_ * 8) + value_);
        it.write(value);
        return it;
    }

    friend bool operator==(Card a, Card b) noexcept {
        return a.who_ == b.who_ && a.value_ == b.value_;
    }
    friend bool operator!=(Card a, Card b) noexcept {
        return !(a == b);
    }
private:
    Color who_ : 3;
    int8_t value_ : 5;
};

struct Column {
    explicit Column() = default;
    bool empty() const { return size_ == 0; }
    int size() const { return size_; }
    void emplace_back(Card card) { assert(size_ < 28); cards_[size_++] = card; }
    const Card& topmost() const { assert(1 <= size_); return cards_[size_-1]; }
    Card operator[](int i) const { assert(0 <= i && i < size_); return cards_[i]; }
private:
    signed char size_ = 0;
    Card cards_[28];
};

struct Board {
    std::vector<Column> columns_;

public:
    explicit Board() = default;
    explicit Board(std::vector<std::vector<Card>> cols) {
        columns_.resize(cols.size());
        for (int i=0; i < int(cols.size()); ++i) {
            for (const Card& card : cols[i]) {
                columns_[i].emplace_back(card);
            }
        }
    }

    void populate_unseen_cards(int8_t (&unseen_cards)[2][8]) {
        for (int w=0; w < 2; ++w) {
            unseen_cards[w][0] = 0;
            for (int v=1; v <= 7; ++v) {
                unseen_cards[w][v] = 2;
            }
        }
        for (const Column& col : columns_) {
            for (int i=0; i < col.size(); ++i) {
                const Card& card = col[i];
                assert(card.color() != Nobody);
                int8_t& cell = unseen_cards[card.color()][card.value()];
                cell -= 1;
                assert(cell >= 0);
            }
        }
    }

    int count_columns() const { return columns_.size(); }

    void apply_in_place(int column, Card card) {
        if (column == -1) {
            columns_.emplace_back();
            std::rotate(columns_.begin(), columns_.end()-1, columns_.end());
            assert(columns_.front().empty());
            columns_.front().emplace_back(card);
        } else if (column == columns_.size()) {
            columns_.emplace_back();
            columns_.back().emplace_back(card);
        } else {
            assert(0 <= column && column < columns_.size());
            columns_[column].emplace_back(card);
        }
    }

    Board apply(int column, Card card) const {
        Board next = *this;
        next.apply_in_place(column, card);
        return next;
    }

    std::string toString() const {
        int max_y = 2;
        for (int x=0; x < columns_.size(); ++x) {
            max_y = std::max(max_y, columns_[x].size());
        }
        std::string result;
        for (int y = max_y; y >= 0; --y) {
            result += "..";
            for (int x = 0; x < columns_.size(); ++x) {
                result += ' ';
                result += cardAt(x, y).toString();
            }
            result += " ..\n";
        }
        return result;
    }

    nibble_writer toPacked(nibble_writer it) const {
        for (int i=0; i < int(columns_.size()); ++i) {
            const Column& col = columns_[i];
            for (int j=0; j < col.size(); ++j) {
                it = col[j].toPacked(it);
            }
            it = Card().toPacked(it);
        }
        return it;
    }

private:
    Card cardAt(int x, int y) const {
        if (0 <= x && x < columns_.size()) {
            if (0 <= y && y < columns_[x].size()) {
                return columns_[x][y];
            }
        }
        return Card();
    }

    bool is_vertical_win_involving(int x, Color who) const {
        int sum = 0;
        for (int y = columns_[x].size() - 1; y >= 0; --y) {
            Card card = cardAt(x, y);
            if (card.color() != who) break;
            sum += card.value();
        }
        return sum >= 15;
    }
    bool is_horizontal_win_involving(int column, Color who) const {
        int sum = 0;
        int y = columns_[column].size() - 1;
        for (int x = column; x >= 0; --x) {
            Card card = cardAt(x, y);
            if (card.color() != who) break;
            sum += card.value();
        }
        for (int x = column+1; x < columns_.size(); ++x) {
            Card card = cardAt(x, y);
            if (card.color() != who) break;
            sum += card.value();
        }
        return sum >= 15;
    }
    bool is_slash_win_involving(int x, Color who) const {
        int sum = 0;
        int y = columns_[x].size() - 1;
        for (int d = 0; true; ++d) {
            Card card = cardAt(x+d, y+d);
            if (card.color() != who) break;
            sum += card.value();
        }
        for (int d = 1; true; ++d) {
            Card card = cardAt(x-d, y-d);
            if (card.color() != who) break;
            sum += card.value();
        }
        return sum >= 15;
    }
    bool is_backslash_win_involving(int x, Color who) const {
        int sum = 0;
        int y = columns_[x].size() - 1;
        for (int d = 0; true; ++d) {
            Card card = cardAt(x+d, y-d);
            if (card.color() != who) break;
            sum += card.value();
        }
        for (int d = 1; true; ++d) {
            Card card = cardAt(x-d, y+d);
            if (card.color() != who) break;
            sum += card.value();
        }
        return sum >= 15;
    }

public:
    bool is_win_involving(int column, Card card) const {
        if (column == -1) {
            column = 0;
        }
        assert(columns_[column].topmost() == card);
        Color who = card.color();
        return (
            is_vertical_win_involving(column, who) ||
            is_horizontal_win_involving(column, who) ||
            is_slash_win_involving(column, who) ||
            is_backslash_win_involving(column, who)
        );
    }

    std::pair<bool, int> must_respond_to_threat(Card card) const {
        std::pair<bool, int> result = { false, 0 };
        for (int column = -1; column <= int(columns_.size()); ++column) {
            Board next = apply(column, card);
            if (next.is_win_involving(column, card)) {
                if (result.first) {
                    // There are two threats! Checkmate!
                    return { true, -2 };
                } else {
                    result = { true, column };
                }
            }
        }
        return result;
    }
};

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
