#include "filtering-program.h"
#include <chrono> // allows to count time
#include <functional> // allows for passing the original variable to a thread instead of its copy std:ref
#include <iostream> // allows for printing to the terminal std:cout
#include <thread> // allows for creating and running threads std::thread



// function which calls the filterring method for every event, designed for multiple threads, since it takes in only a portion of the events, based on first and last arguments
void filter_range(std::vector<std::vector<uint16_t>>& readings, std::vector<uint32_t>& checksum, std::size_t first, std::size_t last, std::size_t& kept) {
    kept = 0;

    // filtering function applied to every event
    for (std::size_t e = first; e < last; e = e + 1) {
        if (filter_event(readings[e], checksum[e])) {
            kept = kept + 1;
        }
    }
}



// the main method
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

    // make an identical copy of the events and checksum list so each test filters its own fresh events
    std::vector<std::vector<uint16_t>> readings_copy = readings;
    std::vector<uint32_t> checksum_copy = checksum;

    // the size of all th event data that goes through in MB used for printing later
    double megabytes = NUM_EVENTS * EVENT_READINGS * 2 / 1000000.0;

    //---------------------------------------------------------------------------------------------
    // FILTERING EVENTS
    

        //=============================== 0 Baseline: filter every event in a plain loop on one core ============================

        std::size_t kept_0 = 0; // variable for printing

        // record start time
        auto starttime_0 = std::chrono::steady_clock::now();

        //filtering function applied to every event
        for (std::size_t e = 0; e < NUM_EVENTS; e = e + 1) {
            if (filter_event(readings[e], checksum[e])) {
                kept_0 = kept_0 + 1;
            }
        }

        // record end time
        auto endtime_0 = std::chrono::steady_clock::now();

        // calculate the duration from start time to end time
        std::chrono::duration<double> DurationFiltering_0 = endtime_0 - starttime_0;


        //========== 1 Multithreaded: split filtering events across threads which run simultaneously on multiple cores =============

        // starts a list which will hold the running threads 
        std::vector<std::thread> threads;
        // a list which stores the number of kept events for each thread 
        std::vector<std::size_t> kept_per_thread(NUM_THREADS);

        // record start time
        auto starttime_1 = std::chrono::steady_clock::now();

        // loop runs once per thread, with every cycle a thread is created
        for (unsigned t = 0; t < NUM_THREADS; ++t) {
            // calculates the position of the first event
            std::size_t first = t * NUM_EVENTS / NUM_THREADS;
            // calculates the position of the last event, the last event for one thread will overlap with the first event for another thread, but the filter_range takes care of that
            std::size_t last = (t + 1) * NUM_EVENTS / NUM_THREADS; 

            // push_back adds a new member to the threads list and resizes it as well so it can dynamically add elements to a list
            // std:thread creates a thread with the first argument being the method each of the threads will run
            // std:ref means it will take the original instead of making a copy
            threads.push_back(std::thread(filter_range, std::ref(readings_copy), std::ref(checksum_copy), first, last, std::ref(kept_per_thread[t])));
        }

        // the method waits from executing the rest of the code until every thread has finished, it actually waits for the first thread, and by that time the rest would probably be done
        for (unsigned t = 0; t < NUM_THREADS; ++t) {
            threads[t].join();
        }

        // record end time
        auto endtime_1 = std::chrono::steady_clock::now();

        // calculate the duration from start and end time
        std::chrono::duration<double> DurationFiltering_1 = endtime_1 - starttime_1;

        // initialize a variable to store the number of events that were kept
        std::size_t kept_1 = 0;
        // traverse the kept_per_thread list and add every element and save under the kept variable
        for (unsigned t = 0; t < NUM_THREADS; t = t + 1) {
            kept_1 = kept_1 + kept_per_thread[t];
        }


        //================================================== Performance Calculations ================================================

        // counting speedup between single and multi threaded experiment
        double speedup = DurationFiltering_0.count() / DurationFiltering_1.count();

        // Amdahl: speedup = 1 / (f + (1 - f) / N), solved for f gives f = (N / speedup - 1) / (N - 1)
        // calculating the serial fraction for multithreaded experiment using Amdahl's Law
        double serial_fraction = (static_cast<double>(NUM_THREADS) / speedup - 1.0) / (NUM_THREADS - 1.0);
        // parallel proportion is the remaining part of the work that benefits from more threads
        double parallel_proportion = 1.0 - serial_fraction;

        // predicting the speedup on 28 cores from the measured serial fraction using Amdahl's Law
        constexpr unsigned PREDICTED_CORES = 28;
        double predicted_speedup = 1.0 / (serial_fraction + parallel_proportion / PREDICTED_CORES);

        //============================================================ Printing ======================================================
        

        // print for single threaded 0
        std::cout << "Experiment 0: baseline (one core, no threads)\n";
        std::cout << "Kept " << kept_0 << " of " << NUM_EVENTS << " events (" << 100.0 * kept_0 / NUM_EVENTS << "%)\n";
        std::cout << "Time: " << DurationFiltering_0.count() << " s (" << megabytes / DurationFiltering_0.count() << " MB/s)\n";

        // print for multithreaded 1
        std::cout << "Experiment 1: scaling with " << NUM_THREADS << " threads\n";
        std::cout << "Kept " << kept_1 << " of " << NUM_EVENTS << " events (" << 100.0 * kept_1 / NUM_EVENTS << "%)\n";
        std::cout << "Time: " << DurationFiltering_1.count() << " s (" << megabytes / DurationFiltering_1.count() << " MB/s)\n";

        // print for calculations
        std::cout << "Speedup: " << speedup << "x\n";
        std::cout << "Serial fraction (Amdahl): " << serial_fraction * 100 << "%\n";
        std::cout << "Parallel proportion (Amdahl): " << parallel_proportion * 100 << "%\n";
        std::cout << "Predicted speedup on " << PREDICTED_CORES << " cores (Amdahl): " << predicted_speedup << "x\n";
   
    return 0;
}