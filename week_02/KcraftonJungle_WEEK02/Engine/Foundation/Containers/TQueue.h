#pragma once
#include <queue>

template <typename T>
using TQueue = std::queue<T>;

template<typename T>
using TPriorityQueue = TQueue<T, EQueueMode::Spsc>;