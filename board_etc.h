#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
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
        if (who_ == Nobody) {
            it.write(0);
        } else {
            it.write(value_ + 8 * who_);
        }
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

    nibble_writer toPacked(nibble_writer it, bool flipHorizontal) const {
        for (int i=0; i < int(columns_.size()); ++i) {
            const Column& col = flipHorizontal ? columns_[columns_.size() - i - 1] : columns_[i];
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

    struct ForcedMove {
        bool is_forced;
        bool is_double_threat;
        int move;
    };

    ForcedMove must_respond_to_threat(Card card) const {
        ForcedMove result = { false, false, 0 };
        for (int column = -1; column <= int(columns_.size()); ++column) {
            Board next = apply(column, card);
            if (next.is_win_involving(column, card)) {
                if (result.is_forced) {
                    // There are two threats! Checkmate!
                    return { true, true, result.move };
                } else {
                    result = { true, false, column };
                }
            }
        }
        return result;
    }
};
