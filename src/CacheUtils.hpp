#pragma once

#include <vector>
#include <cstdint>
#include <iostream>

constexpr int32_t CACHE_WAY = 4;
constexpr int32_t CACHE_TAG_LEN = 9;
constexpr int32_t CACHE_INDEX_LEN = 3;
constexpr int32_t CACHE_OFFSET_LEN = 6;
constexpr int32_t CACHE_SIZE = 2048;
constexpr int32_t CACHE_LINE_SIZE = 64;
constexpr int32_t CACHE_LINE_COUNT = 32;
constexpr int32_t CACHE_SETS = 8;

enum class Replacement {
    LRU,
    BIT_P_LRU,
    BOTH
};
