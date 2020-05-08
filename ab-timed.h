#pragma once

#include "state.h"
#include <atomic>
#include <chrono>
#include <utility>

using LeafEvaluationFunction = double(*)(const State&);

double simplest_eval(const State& s);

extern int recursively_scheduled_tasks;
extern int recursively_evaluated_tasks;
extern std::atomic<int> max_search_depth;

std::pair<double, int> recursively_evaluate(LeafEvaluationFunction eval, const State& s, std::chrono::milliseconds timeout);
