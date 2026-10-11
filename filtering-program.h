// This file contains all shared declarations for a simulated event filtering program (scaled down version of what PrecisionBeam works on)

#pragma once // ensures that this file is included (loaded) only once in a single compilation, ensuring that the data-types wont be redefined causing errors

//including the necessary libraries
#include <cstddef> //gives the std::size_t type, which is an unsigned (positive and 0) integer type used for sizes and counts
#include <cstdint> //gives the std::uint16_t and std::uint32_t types, which are fixed-width unsigned integer types
#include <vector>

// Experiment settings: change these, recompile, run again
// 0 = baseline - single thread
// 1 = scaling - multi thread
// 2 = buffer - multi thread with output buffer sized above L2 cache
// 3 = shared - multi thread with 2 output buffers on each core instead of a shared buffer for 2 threads on one core
constexpr int EXPERIMENT = 1;

constexpr unsigned NUM_THREADS = 12;   // number of threads to be used for multithread experiments

// setting fixed unchangeable variables, holding general specifications, which will be used throughout the program
// constexpr means they cannot be changed while the program is running, and it also guarantees the compiler knows this value when compiling

constexpr std::size_t NUM_EVENTS = 30000;
// each event is set to 20,000 readings = 40 kB per event
constexpr std::size_t EVENT_READINGS = 20000;
// a vaild event that should be kept will get 12k strong readings (thus its kept when compared against the 10k threshold)
constexpr std::size_t SIGNAL_STRONG = 12000;
// an invalid event that will be discarded will get 2k strong readings (thus it is discared when compared against the 10k threshold)
constexpr std::size_t BACKGROUND_STRONG = 2000;
// readings below this are noise
constexpr uint16_t THRESHOLD = 1000;
// events with fewer strong readings are discarded
constexpr std::size_t MIN_STRONG = 10000;

// function declaration (done for the compiler to be informed of the existence of these functions and that the structs will be used there)
uint32_t sum_checksum(const std::vector<uint16_t>& event);
void generate_events(
    std::vector<std::vector<uint16_t>>& readings,
    std::vector<uint32_t>& checksum);
bool filter_event(std::vector<uint16_t>& event, uint32_t stored_checksum);
