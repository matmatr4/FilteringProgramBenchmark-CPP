#include "filtering-program.h"
#include <chrono> 
#include <iostream>

int main() {

    //-------------------------------------------------------------------------------
    // GENERATING EVENTS
    // nested array which will store all events and readings, readings[i] would show an array containing all readings inside an event, readings[i][i] will show a particular reading of an event
    // uint16_t since each reading is maximum 12-bits
    std::vector<std::vector<uint16_t>> readings;
    // array which stores the checksum for each event, so checksum[i] stores the checksum for event i (so corresponds to readings[i])
    // uinit32_t because checksum adds all readings together, so 20k times 4,095 gives 81.9 million which is the highest possible, so not enough for 16 but enough for 32 bit which gives up to 4.29 billion
    std::vector<uint32_t> checksum;

    // readings = [
    //     [ 412, 2731, 87, 3390, 655, ... ],    // event 0    (signal: 12,000 readings >= 1000)
    //     [ 30, 655, 1904, 12, 498, ... ],      // event 1    (background: 2,000 readings >= 1000)
    //     [ 799, 2048, 4011, 233, 61, ... ],    // event 2    (signal)
    //     ...
    //     [ 215, 88, 3076, 540, 9, ... ]        // event 5999
    // ]
    //
    // checksum = [ 34120577, 13048211, 33987402, ..., 12961530 ]
    //              // event 0   event 1    event 2         event 5999


    // print
    std::cout << "Generating " << NUM_EVENTS << " events...\n";

    // record start time
    auto gen_starttime = std::chrono::steady_clock::now();

    // function to generate events and fill the two lists above
    generate_events(readings, checksum);

    // record end time
    auto gen_endtime = std::chrono::steady_clock::now();

    // calculate the duration from start time and end time
    std::chrono::duration<double> DurationGeneration = gen_endtime - gen_starttime;

    // print
    std::cout << "Generation took " << DurationGeneration.count() << " s\n\n";

    // the size of all th event data that goes through in MB used for printing later
    double megabytes = NUM_EVENTS * EVENT_READINGS * 2 / 1000000.0;

    //------------------------------------------------------------------------------
    // FILTERING EVENTS
    if (EXPERIMENT == 0) {
        // Baseline: filter every event in a plain loop on one core

        std::cout << "Experiment 0: baseline (one core, no threads)\n";
        std::size_t kept = 0; // variable for printing

        // record start time
        auto starttime = std::chrono::steady_clock::now();

        //filtering function
        for (std::size_t e = 0; e < NUM_EVENTS; e = e + 1) {
            if (filter_event(readings[e], checksum[e])) {
                kept = kept + 1;
            }
        }

        // record end time
        auto endtime = std::chrono::steady_clock::now();

        // calculate the duration from start time to end time
        std::chrono::duration<double> DurationFiltering = endtime - starttime;

        // print
        std::cout << "Kept " << kept << " of " << NUM_EVENTS << " events (" << 100.0 * kept / NUM_EVENTS << "%)\n";
        std::cout << "Time: " << DurationFiltering.count() << " s (" << megabytes / DurationFiltering.count() << " MB/s)\n";

    } else if (EXPERIMENT == 1) {
        // Scaling: filter with NUM_THREADS threads (step 3)

    } else if (EXPERIMENT == 2) {
        // Buffer: filter with a BUFFER_BYTES output buffer per thread (step 4)

    } else if (EXPERIMENT == 3) {
        // Shared: one shared output buffer instead of one per thread (step 5)

    } else {
        std::cout << "Unknown experiment number\n";
    }
    return 0;
}