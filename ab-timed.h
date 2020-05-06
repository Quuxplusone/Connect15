#pragma once

#include "connect15.h"
#include <chrono>
#include <utility>

using LeafEvaluationFunction = double(*)(const State&);

double simplest_eval(const State& s);

std::pair<double, int> recursively_evaluate(LeafEvaluationFunction eval, const State& s, std::chrono::milliseconds timeout);