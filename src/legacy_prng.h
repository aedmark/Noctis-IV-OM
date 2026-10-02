#pragma once

#include <cstdint>

extern std::uint32_t flat_rnd_seed;

void fast_srand(std::uint32_t seed);
std::int32_t fast_random(std::int32_t mask);
std::int16_t ranged_fast_random(std::int16_t range);
float flandom();
float fast_flandom();
