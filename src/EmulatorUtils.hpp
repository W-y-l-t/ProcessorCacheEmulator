#pragma once

#include <iostream>

struct Counters {
    size_t hit;
    size_t miss;

    Counters() : hit(0), miss(0) {}
};
