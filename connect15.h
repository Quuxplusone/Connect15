#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <numeric>
#include <string>
#include <vector>

enum Color {
    Red = 0,
    Black = 1,
    Nobody = 2,
};

struct Card {
    explicit Card() : who_(Nobody), value_(0) {}
    explicit Card(Color who, int value) : who_(who), value_(value) {}
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

    int count_threats(Card card) const {
        int count = 0;
        for (int column = -1; column <= int(columns_.size()); ++column) {
            Board next = apply(column, card);
            if (next.is_win_involving(column, card)) {
                ++count;
                if (count >= 2) break;
            }
        }
        return count;
    }
};

struct State {
    template<class Random>
    static State initial(Random rand) {
        State s;
        for (int v=1; v <= 7; ++v) {
            s.unseen_cards_[Red][v] = 2;
            s.unseen_cards_[Black][v] = 2;
        }
        s.top_card_[Red] = s.draw_next_card(rand, Red);
        s.top_card_[Black] = s.draw_next_card(rand, Black);
        return s;
    }

    template<class Random>
    Card draw_next_card(Random rand, Color who) {
        int sum = std::accumulate(std::begin(unseen_cards_[who]), std::end(unseen_cards_[who]), 0);
        if (sum == 0) {
            return Card();
        }
        assert(0 < sum && sum <= 14);
        int k = rand() % sum;
        int v = 0;
        while (k >= 0) {
            k -= unseen_cards_[who][++v];
        }
        assert(1 <= v && v <= 7);
        assert(unseen_cards_[who][v] >= 1);
        unseen_cards_[who][v] -= 1;
        return Card(who, v);
    }

    std::string toString() const {
        std::string result = board_.toString() + "\n";
        result += "Red's top card: " + top_card_[Red].toString() + "\n";
        result += "Black's top card: " + top_card_[Black].toString();
        return result;
    }

    Color whose_move() const {
        return who_;
    }

    bool is_tie_game() const {
        return top_card_[who_].color() == Nobody;
    }

    template<class Random>
    bool apply_in_place(Random rand, int column) {
        Card card = top_card_[who_];
        board_.apply_in_place(column, card);
        top_card_[who_] = draw_next_card(rand, who_);
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

private:
    explicit State() = default;

    Board board_;
    int8_t unseen_cards_[2][8] = {};
    Card top_card_[2];
    Color who_ = Red;
};
