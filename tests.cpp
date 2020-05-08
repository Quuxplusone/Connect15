#include <chrono>
#include <iostream>
#include <string>

#include "connect15.h"
#include "ab-timed.h"

void test1() {
    auto b = Board({
        { Card("3r"), Card("6b"), Card("5r"), Card("4r"), Card("1b"), Card("2r") },
        { Card("1r"), Card("4b"), Card("6r"), Card("3b"), Card("2b"), Card("6r"), Card("4b"), Card("3r"), Card("5b"), Card("4r"), Card("7b"), Card("6b"), Card("2r"), Card("5b") },
    });
    auto s = State(Black, Card("7r"), Card("5b"), std::move(b));

    std::string swho = ((s.active_player() == Red) ? "Red" : "Black");
    auto vm = recursively_evaluate(simplest_eval, s, std::chrono::milliseconds(150));
    std::cout << s.toString() << "\n";
    std::cout << "AI thinks " << swho << "'s best move is " << vm.second << " (value " << vm.first << ").\n";
    printf("Scheduled %d tasks, ran %d tasks, search depth %d.\n",
           recursively_scheduled_tasks, recursively_evaluated_tasks, max_search_depth.load());
}

void test2() {
    auto b = Board({
        { Card("3r"), Card("6b"), Card("5r"), Card("4r"), Card("1b"), Card("2r") },
        { Card("1r"), Card("4b"), Card("6r"), Card("3b"), Card("2b"), Card("6r"), Card("4b"), Card("3r"), Card("5b"), Card("4r"), Card("7b"), Card("6b"), Card("2r"), Card("5b") },
    });
    auto s = State(Red, Card("7r"), Card("4b"), std::move(b));

    std::string swho = ((s.active_player() == Red) ? "Red" : "Black");
    auto vm = recursively_evaluate(simplest_eval, s, std::chrono::milliseconds(150));
    std::cout << s.toString() << "\n";
    std::cout << "AI thinks " << swho << "'s best move is " << vm.second << " (value " << vm.first << ").\n";
    printf("Scheduled %d tasks, ran %d tasks, search depth %d.\n",
           recursively_scheduled_tasks, recursively_evaluated_tasks, max_search_depth.load());
}

int main() {
    test2();
}
