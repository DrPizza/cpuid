#pragma once

#include <cstdint>

extern uint64_t tick_rate;
extern uint64_t measurement_overhead;

uint64_t get_actual_frequency();
uint64_t get_measurement_overhead();
