#include "filtering-program.h"
#include <chrono> // allows to count time
#include <functional> // allows for passing the original variable to a thread instead of its copy std:ref
#include <iostream> // allows for printing to the terminal std:cout
#include <thread> // allows for creating and running threads std::thread
#include <mutex>
#include <atomic>


// experiment 1 and 0

// function which calls the filterring method for every event, designed for multiple threads, since it takes in only a portion of the events, based on first and last arguments
void filter_range(std::vector<std::vector<uint16_t>>& readings, std::vector<uint32_t>& checksum, std::size_t first, std::size_t last, std::size_t& kept1) {
    std::size_t kept = 0;

    // filtering function applied to every event
    for (std::size_t e = first; e < last; ++e) {
        if (filter_event(readings[e], checksum[e])) {
            kept = kept + 1;
        }
    }

    kept1 = kept;
}





// experiment 2

std::vector<std::vector<uint16_t>> filtered_readings2;
std::mutex shared;

void mutex_filter_range(
    std::vector<std::vector<uint16_t>>& readings, 
    std::vector<uint32_t>& checksum, 
    std::size_t first, 
    std::size_t last, 
    std::size_t& kept2) 

{
    std::size_t kept = 0;

    // filtering function applied to every event
    for (std::size_t e = first; e < last; ++e) {
        if (filter_event(readings[e], checksum[e])) {
            kept = kept + 1;
            // saves event into the shared structure
            addToShared(readings[e], filtered_readings2);
        }
    }

    kept2 = kept;
}

void addToShared(std::vector<uint16_t>& event, std::vector<std::vector<uint16_t>>& filtered_events) {

    // same lock behaviour as with .lock() and .unlock() but with error handling
    std::lock_guard<std::mutex> lock(shared); 
    filtered_events.push_back(std::move(event)); 
    // move here ensure the data is not copied, but moved, so event is turned empty, that way its quicker and thousands of readings dont need to be copied
    
}




// experiment 3

std::vector<std::vector<uint16_t>> filtered_readings3;

void buffered_mutex_filter_range(
    std::vector<std::vector<uint16_t>>& readings, 
    std::vector<uint32_t>& checksum, 
    std::size_t first, 
    std::size_t last, 
    std::size_t& kept3) 

{
    std::size_t kept = 0;

    std::vector<std::vector<uint16_t>> filtered_buffer;
    filtered_buffer.reserve(BUFFER_SIZE);

    // filtering function applied to every event
    for (std::size_t e = first; e < last; ++e) {
        if (filter_event(readings[e], checksum[e])) {
            kept = kept + 1;

            // if using buffer in bytes size, additional check for event not fitting into the whole buffer should be added

            // moving readings to the buffer
            filtered_buffer.push_back(std::move(readings[e]));
            
            if (filtered_buffer.size() == BUFFER_SIZE) {
                // flushes the current buffer into the shared structure
                addToSharedBuffer(filtered_buffer, filtered_readings3);
            }
        }
    }

    // flush the leftovers
    if (!filtered_buffer.empty()) {
        addToSharedBuffer(filtered_buffer, filtered_readings3);
    }

    kept3 = kept;
}

void addToSharedBuffer(std::vector<std::vector<uint16_t>>& buffer, std::vector<std::vector<uint16_t>>& filtered_events) {

    // same lock behaviour as with .lock() and .unlock() but with error handling
    std::lock_guard<std::mutex> lock(shared); 
    for (std::size_t e = 0; e < buffer.size(); ++e) {

        filtered_events.push_back(std::move(buffer[e])); 
        // it moves whats inside the buffer, deleting it, and leaving it only in filtered_events, which is desired
    
    }

    buffer.clear(); // to remove empty slots
}





// experiment 4

std::vector<std::vector<uint16_t>> filtered_readings4;
std::atomic<std::size_t> next_free_shared_out_slot;

void atomic_filter_range(
    std::vector<std::vector<uint16_t>>& readings, 
    std::vector<uint32_t>& checksum, 
    std::size_t first, 
    std::size_t last, 
    std::size_t& kept4) 
{

    std::size_t kept = 0;

    // filtering function applied to every event
    for (std::size_t e = first; e < last; ++e) {
        if (filter_event(readings[e], checksum[e])) {
            kept = kept + 1;
            // saves event into the shared structure
            addToSharedAtomic(readings[e], filtered_readings4);
        }
    }

    kept4 = kept;

}

void addToSharedAtomic(std::vector<uint16_t>& event, std::vector<std::vector<uint16_t>>& filtered_events) {

    std::size_t current_shared_out_slot = next_free_shared_out_slot.fetch_add(1); // saving, adding and overwriting the variable needs to be in one line
    
    filtered_events[current_shared_out_slot] = (std::move(event)); 

}




// experiment 5

std::vector<std::vector<uint16_t>> filtered_readings5;

void buffered_atomic_filter_range(
    std::vector<std::vector<uint16_t>>& readings, 
    std::vector<uint32_t>& checksum, 
    std::size_t first, 
    std::size_t last, 
    std::size_t& kept5) 

{
    std::size_t kept = 0;

    std::vector<std::vector<uint16_t>> filtered_buffer;
    filtered_buffer.reserve(BUFFER_SIZE);

    // filtering function applied to every event
    for (std::size_t e = first; e < last; ++e) {
        if (filter_event(readings[e], checksum[e])) {
            kept = kept + 1;

            // if using buffer in bytes size, additional check for event not fitting into the whole buffer should be added

            // moving readings to the buffer
            filtered_buffer.push_back(std::move(readings[e]));
            
            if (filtered_buffer.size() == BUFFER_SIZE) {
                // flushes the current buffer into the shared structure
                addToSharedAtomicBuffer(filtered_buffer, filtered_readings5);
            }
        }
    }

    // flush the leftovers
    if (!filtered_buffer.empty()) {
        addToSharedAtomicBuffer(filtered_buffer, filtered_readings5);
    }

    kept5 = kept;
}

void addToSharedAtomicBuffer(std::vector<std::vector<uint16_t>>& buffer, std::vector<std::vector<uint16_t>>& filtered_events) {

    for (std::size_t e = 0; e < buffer.size(); ++e) {

        std::size_t current_shared_out_slot = next_free_shared_out_slot.fetch_add(1); // saving, adding and overwriting the variable needs to be in one line
        filtered_events[current_shared_out_slot] = (std::move(buffer[e])); 
        // it moves whats inside the buffer, deleting it, and leaving it only in filtered_events, which is desired
    
    }

    buffer.clear(); // to remove empty slots
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
    std::cout << "Generation took " << DurationGeneration.count() << " s\n\n\n";
    std::cout << "------------------------------------------------------------\n\n\n";

    // make an identical copy of the events and checksum list so each test filters its own fresh events
    std::vector<std::vector<uint16_t>> readings_copy = readings;
    std::vector<uint32_t> checksum_copy = checksum;
    std::vector<std::vector<uint16_t>> readings_copy2 = readings;
    std::vector<uint32_t> checksum_copy2 = checksum;
    std::vector<std::vector<uint16_t>> readings_copy3 = readings;
    std::vector<uint32_t> checksum_copy3 = checksum;
    std::vector<std::vector<uint16_t>> readings_copy4 = readings;
    std::vector<uint32_t> checksum_copy4 = checksum;
    std::vector<std::vector<uint16_t>> readings_copy5 = readings;
    std::vector<uint32_t> checksum_copy5 = checksum;

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





        //=============================== 2 Mutex: split across threads with a shared structure ============================

        // starts a list which will hold the running threads 
        std::vector<std::thread> threads2;
        // a list which stores the number of kept events for each thread 
        std::vector<std::size_t> kept_per_thread2(NUM_THREADS);

        // clears the array of filtered events, between runs
        filtered_readings2.clear();

        // reserves space for this array
        filtered_readings2.reserve(NUM_EVENTS);
        // Without reserve, the vector starts empty and doubles its capacity each time it fills (1, 2, 4, … 1,024), so for 900 events it reallocates about 11 times, each time moving every stored event while the other threads wait on the mutex. reserve avoids this by allocating all the space once, before timing starts.

        // record start time
        auto starttime_2 = std::chrono::steady_clock::now();

        for (unsigned t = 0; t < NUM_THREADS; ++t) {
            // calculates the position of the first event
            std::size_t first = t * NUM_EVENTS / NUM_THREADS;
            // calculates the position of the last event, the last event for one thread will overlap with the first event for another thread, but the filter_range takes care of that
            std::size_t last = (t + 1) * NUM_EVENTS / NUM_THREADS; 

            // push_back adds a new member to the threads list and resizes it as well so it can dynamically add elements to a list
            // std:thread creates a thread with the first argument being the method each of the threads will run
            // std:ref means it will take the original instead of making a copy
            threads2.push_back(std::thread(mutex_filter_range, std::ref(readings_copy2), std::ref(checksum_copy2), first, last, std::ref(kept_per_thread2[t])));
        }

        for (unsigned t = 0; t < NUM_THREADS; ++t) {
           threads2[t].join();
        }

        // record end time
        auto endtime_2 = std::chrono::steady_clock::now();

        // calculate the duration from start time to end time
        std::chrono::duration<double> DurationFiltering_2 = endtime_2 - starttime_2;

        std::size_t kept_2 = 0;
        // traverse the kept_per_thread list and add every element and save under the kept variable
        for (unsigned t = 0; t < NUM_THREADS; t = t + 1) {
            kept_2 = kept_2 + kept_per_thread2[t];
        }





        //=============================== 3 Mutex: split across threads with a shared structure and each its own output buffer ============================

        // starts a list which will hold the running threads 
        std::vector<std::thread> threads3;
        // a list which stores the number of kept events for each thread 
        std::vector<std::size_t> kept_per_thread3(NUM_THREADS);

        // clears the array of filtered events, between runs
        filtered_readings3.clear();

        // reserves space for this array
        filtered_readings3.reserve(NUM_EVENTS);
        // Without reserve, the vector starts empty and doubles its capacity each time it fills (1, 2, 4, … 1,024), so for 900 events it reallocates about 11 times, each time moving every stored event while the other threads wait on the mutex. reserve avoids this by allocating all the space once, before timing starts.

        // record start time
        auto starttime_3 = std::chrono::steady_clock::now();

        for (unsigned t = 0; t < NUM_THREADS; ++t) {
            // calculates the position of the first event
            std::size_t first = t * NUM_EVENTS / NUM_THREADS;
            // calculates the position of the last event, the last event for one thread will overlap with the first event for another thread, but the filter_range takes care of that
            std::size_t last = (t + 1) * NUM_EVENTS / NUM_THREADS; 

            // push_back adds a new member to the threads list and resizes it as well so it can dynamically add elements to a list
            // std:thread creates a thread with the first argument being the method each of the threads will run
            // std:ref means it will take the original instead of making a copy
            threads3.push_back(std::thread(buffered_mutex_filter_range, std::ref(readings_copy3), std::ref(checksum_copy3), first, last, std::ref(kept_per_thread3[t])));
        }

        for (unsigned t = 0; t < NUM_THREADS; ++t) {
           threads3[t].join();
        }

        // record end time
        auto endtime_3 = std::chrono::steady_clock::now();

        // calculate the duration from start time to end time
        std::chrono::duration<double> DurationFiltering_3 = endtime_3 - starttime_3;

        std::size_t kept_3 = 0;
        // traverse the kept_per_thread list and add every element and save under the kept variable
        for (unsigned t = 0; t < NUM_THREADS; t = t + 1) {
            kept_3 = kept_3 + kept_per_thread3[t];
        }




        
        //=============================== 4 Atomic: split across threads with a shared structure and an atomic variable counter instead of a mutex ============================

        // starts a list which will hold the running threads 
        std::vector<std::thread> threads4;
        // a list which stores the number of kept events for each thread 
        std::vector<std::size_t> kept_per_thread4(NUM_THREADS);

        // clears the array of filtered events, between runs
        filtered_readings4.clear();

        // set the size of the array, since there is no mutex anymore, thus it cannot be dynamic
        filtered_readings4.resize(NUM_EVENTS);

        // save the next free slot to 0, as the filtered array is empty
        next_free_shared_out_slot = 0;

        // record start time
        auto starttime_4 = std::chrono::steady_clock::now();

        for (unsigned t = 0; t < NUM_THREADS; ++t) {
            // calculates the position of the first event
            std::size_t first = t * NUM_EVENTS / NUM_THREADS;
            // calculates the position of the last event, the last event for one thread will overlap with the first event for another thread, but the filter_range takes care of that
            std::size_t last = (t + 1) * NUM_EVENTS / NUM_THREADS; 

            // push_back adds a new member to the threads list and resizes it as well so it can dynamically add elements to a list
            // std:thread creates a thread with the first argument being the method each of the threads will run
            // std:ref means it will take the original instead of making a copy
            threads4.push_back(std::thread(atomic_filter_range, std::ref(readings_copy4), std::ref(checksum_copy4), first, last, std::ref(kept_per_thread4[t])));
        }

        for (unsigned t = 0; t < NUM_THREADS; ++t) {
           threads4[t].join();
        }

        // record end time
        auto endtime_4 = std::chrono::steady_clock::now();

        // calculate the duration from start time to end time
        std::chrono::duration<double> DurationFiltering_4 = endtime_4 - starttime_4;

        std::size_t kept_4 = 0;
        // traverse the kept_per_thread list and add every element and save under the kept variable
        for (unsigned t = 0; t < NUM_THREADS; t = t + 1) {
            kept_4 = kept_4 + kept_per_thread4[t];
        }

        filtered_readings4.resize(next_free_shared_out_slot);





        //=============================== 5 Atomic: split across threads with a shared structure and an atomic variable counter instead of a mutex, and an output buffer ============================

        // starts a list which will hold the running threads 
        std::vector<std::thread> threads5;
        // a list which stores the number of kept events for each thread 
        std::vector<std::size_t> kept_per_thread5(NUM_THREADS);

        // clears the array of filtered events, between runs
        filtered_readings5.clear();

        // set the size of the array, since there is no mutex anymore, thus it cannot be dynamic
        filtered_readings5.resize(NUM_EVENTS);

        // save the next free slot to 0, as the filtered array is empty
        next_free_shared_out_slot = 0;

        // record start time
        auto starttime_5 = std::chrono::steady_clock::now();

        for (unsigned t = 0; t < NUM_THREADS; ++t) {
            // calculates the position of the first event
            std::size_t first = t * NUM_EVENTS / NUM_THREADS;
            // calculates the position of the last event, the last event for one thread will overlap with the first event for another thread, but the filter_range takes care of that
            std::size_t last = (t + 1) * NUM_EVENTS / NUM_THREADS; 

            // push_back adds a new member to the threads list and resizes it as well so it can dynamically add elements to a list
            // std:thread creates a thread with the first argument being the method each of the threads will run
            // std:ref means it will take the original instead of making a copy
            threads5.push_back(std::thread(buffered_atomic_filter_range, std::ref(readings_copy5), std::ref(checksum_copy5), first, last, std::ref(kept_per_thread5[t])));
        }

        for (unsigned t = 0; t < NUM_THREADS; ++t) {
           threads5[t].join();
        }

        // record end time
        auto endtime_5 = std::chrono::steady_clock::now();

        // calculate the duration from start time to end time
        std::chrono::duration<double> DurationFiltering_5 = endtime_5 - starttime_5;

        std::size_t kept_5 = 0;
        // traverse the kept_per_thread list and add every element and save under the kept variable
        for (unsigned t = 0; t < NUM_THREADS; t = t + 1) {
            kept_5 = kept_5 + kept_per_thread5[t];
        }

        filtered_readings5.resize(next_free_shared_out_slot);







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


        // experiment 2 and 3
        // counting speedup between single and multi threaded experiment
        double speedup2 = DurationFiltering_0.count() / DurationFiltering_2.count();
        double speedup3 = DurationFiltering_0.count() / DurationFiltering_3.count();

        // Amdahl: speedup = 1 / (f + (1 - f) / N), solved for f gives f = (N / speedup - 1) / (N - 1)
        // calculating the serial fraction for multithreaded experiment using Amdahl's Law
        double serial_fraction2 = (static_cast<double>(NUM_THREADS) / speedup2 - 1.0) / (NUM_THREADS - 1.0);
        // parallel proportion is the remaining part of the work that benefits from more threads
        double parallel_proportion2 = 1.0 - serial_fraction2;

        double serial_fraction3 = (static_cast<double>(NUM_THREADS) / speedup3 - 1.0) / (NUM_THREADS - 1.0);
        // parallel proportion is the remaining part of the work that benefits from more threads
        double parallel_proportion3 = 1.0 - serial_fraction3;


        // gain from adding a buffer 2/3
        double buffer_gain = DurationFiltering_2.count() / DurationFiltering_3.count();

        // predicting the speedup on 28 cores from the measured serial fraction using Amdahl's Law
        double predicted_speedup2 = 1.0 / (serial_fraction2 + parallel_proportion2 / PREDICTED_CORES);
        double predicted_speedup3 = 1.0 / (serial_fraction3 + parallel_proportion3 / PREDICTED_CORES);


        // experiment 4 and 5
        // counting speedup between single and multi threaded experiment
        double speedup4 = DurationFiltering_0.count() / DurationFiltering_4.count();
        double speedup5 = DurationFiltering_0.count() / DurationFiltering_5.count();

        // Amdahl: speedup = 1 / (f + (1 - f) / N), solved for f gives f = (N / speedup - 1) / (N - 1)
        // calculating the serial fraction for multithreaded experiment using Amdahl's Law
        double serial_fraction4 = (static_cast<double>(NUM_THREADS) / speedup4 - 1.0) / (NUM_THREADS - 1.0);
        // parallel proportion is the remaining part of the work that benefits from more threads
        double parallel_proportion4 = 1.0 - serial_fraction4;

        double serial_fraction5 = (static_cast<double>(NUM_THREADS) / speedup5 - 1.0) / (NUM_THREADS - 1.0);
        // parallel proportion is the remaining part of the work that benefits from more threads
        double parallel_proportion5 = 1.0 - serial_fraction5;


        // gain from adding a buffer 4/5
        double buffer_gain45 = DurationFiltering_4.count() / DurationFiltering_5.count();

        // predicting the speedup on 28 cores from the measured serial fraction using Amdahl's Law
        double predicted_speedup4 = 1.0 / (serial_fraction4 + parallel_proportion4 / PREDICTED_CORES);
        double predicted_speedup5 = 1.0 / (serial_fraction5 + parallel_proportion5 / PREDICTED_CORES);
        



        //============================================================ Printing ====================================================
        

        // print for single threaded 0
        std::cout << "Experiment 0: baseline (one core, no threads)\n";
        std::cout << "Kept " << kept_0 << " of " << NUM_EVENTS << " events (" << 100.0 * kept_0 / NUM_EVENTS << "%)\n";
        std::cout << "Time: " << DurationFiltering_0.count() << " s (" << megabytes / DurationFiltering_0.count() << " MB/s)\n\n";

        // print for multithreaded 1
        std::cout << "Experiment 1: scaling with " << NUM_THREADS << " threads\n";
        std::cout << "Kept " << kept_1 << " of " << NUM_EVENTS << " events (" << 100.0 * kept_1 / NUM_EVENTS << "%)\n";
        std::cout << "Time: " << DurationFiltering_1.count() << " s (" << megabytes / DurationFiltering_1.count() << " MB/s)\n\n";

        // print for calculations
        std::cout << "Speedup: " << speedup << "x\n";
        std::cout << "Serial fraction (Amdahl): " << serial_fraction * 100 << "%\n";
        std::cout << "Parallel proportion (Amdahl): " << parallel_proportion * 100 << "%\n";
        std::cout << "Predicted speedup on " << PREDICTED_CORES << " cores (Amdahl): " << predicted_speedup << "x\n\n\n";

        std::cout << "------------------------------------------------------------\n\n\n";




        // print for experiment 2
        std::cout << "Experiment 2: baseline mutex multithreaded with " << NUM_THREADS << " threads\n";
        std::cout << "Kept " << kept_2 << " of " << NUM_EVENTS << " events (" << 100.0 * kept_2 / NUM_EVENTS << "%)\n";
        std::cout << "Kept in the shared buffer " << filtered_readings2.size() << " of " << NUM_EVENTS << " events (" << 100.0 * filtered_readings2.size() / NUM_EVENTS << "%)\n";
        std::cout << "Time: " << DurationFiltering_2.count() << " s (" << megabytes / DurationFiltering_2.count() << " MB/s)\n\n";


        // print for experiment 3
        std::cout << "Experiment 3: buffered mutex multithreaded with " << NUM_THREADS << " threads\n";
        std::cout << "Kept " << kept_3 << " of " << NUM_EVENTS << " events (" << 100.0 * kept_3 / NUM_EVENTS << "%)\n";
        std::cout << "Kept in the shared buffer " << filtered_readings3.size() << " of " << NUM_EVENTS << " events (" << 100.0 * filtered_readings3.size() / NUM_EVENTS << "%)\n";
        std::cout << "Time: " << DurationFiltering_3.count() << " s (" << megabytes / DurationFiltering_3.count() << " MB/s)\n\n";
    

        // print for calculations 2 and 3
        std::cout << "Speedup for 2: " << speedup2 << "x\n";
        std::cout << "Speedup for 3: " << speedup3 << "x\n\n";

        std::cout << "Serial fraction for experiment 2 (Amdahl): " << serial_fraction2 * 100 << "%\n";
        std::cout << "Parallel proportion for experiment 2 (Amdahl): " << parallel_proportion2 * 100 << "%\n";
        std::cout << "Predicted speedup on " << PREDICTED_CORES << " cores for experiment 2 (Amdahl): " << predicted_speedup2 << "x\n\n";

        std::cout << "Serial fraction for experiment 3 (Amdahl): " << serial_fraction3 * 100 << "%\n";
        std::cout << "Parallel proportion for experiment 3 (Amdahl): " << parallel_proportion3 * 100 << "%\n";
        std::cout << "Predicted speedup on " << PREDICTED_CORES << " cores for experiment 3 (Amdahl): " << predicted_speedup3 << "x\n\n";

        std::cout << "Measured gain from adding an output buffer: " << buffer_gain << "x\n\n\n";

        std::cout << "------------------------------------------------------------\n\n\n";

    



        // print for experiment 4
        std::cout << "Experiment 4: baseline atomic count multithreaded with " << NUM_THREADS << " threads\n";
        std::cout << "Kept " << kept_4 << " of " << NUM_EVENTS << " events (" << 100.0 * kept_4 / NUM_EVENTS << "%)\n";
        std::cout << "Kept in the shared buffer " << filtered_readings4.size() << " of " << NUM_EVENTS << " events (" << 100.0 * filtered_readings4.size() / NUM_EVENTS << "%)\n";
        std::cout << "Time: " << DurationFiltering_4.count() << " s (" << megabytes / DurationFiltering_4.count() << " MB/s)\n\n";


        // print for experiment 5
        std::cout << "Experiment 5: buffered atomic count multithreaded with " << NUM_THREADS << " threads\n";
        std::cout << "Kept " << kept_5 << " of " << NUM_EVENTS << " events (" << 100.0 * kept_5 / NUM_EVENTS << "%)\n";
        std::cout << "Kept in the shared buffer " << filtered_readings5.size() << " of " << NUM_EVENTS << " events (" << 100.0 * filtered_readings5.size() / NUM_EVENTS << "%)\n";
        std::cout << "Time: " << DurationFiltering_5.count() << " s (" << megabytes / DurationFiltering_5.count() << " MB/s)\n\n";
        

        // print for calculations 4 and 5
        std::cout << "Speedup for 4: " << speedup4 << "x\n";
        std::cout << "Speedup for 5: " << speedup5 << "x\n\n";

        std::cout << "Serial fraction for experiment 4 (Amdahl): " << serial_fraction4 * 100 << "%\n";
        std::cout << "Parallel proportion for experiment 4 (Amdahl): " << parallel_proportion4 * 100 << "%\n";
        std::cout << "Predicted speedup on " << PREDICTED_CORES << " cores for experiment 4 (Amdahl): " << predicted_speedup4 << "x\n\n";

        std::cout << "Serial fraction for experiment 5 (Amdahl): " << serial_fraction5 * 100 << "%\n";
        std::cout << "Parallel proportion for experiment 5 (Amdahl): " << parallel_proportion5 * 100 << "%\n";
        std::cout << "Predicted speedup on " << PREDICTED_CORES << " cores for experiment 5 (Amdahl): " << predicted_speedup5 << "x\n\n";

        std::cout << "Measured gain from adding an output buffer: " << buffer_gain45 << "x\n\n\n";

        std::cout << "------------------------------------------------------------\n\n\n";

        // print comparison of mutex and atomic
        std::cout << "Comparing Speedup in Different Scenarios: \n\n";
        std::cout << "Mutex vs atomic, per event (2 vs 4): " << DurationFiltering_2.count() / DurationFiltering_4.count() << "x\n";
        std::cout << "Mutex vs atomic, buffered (3 vs 5): " << DurationFiltering_3.count() / DurationFiltering_5.count() << "x\n";
        
        
        double Fastest = std::min({DurationFiltering_2.count(), DurationFiltering_3.count(), DurationFiltering_4.count(), DurationFiltering_5.count()});
        std::string FastestName;

        if (Fastest == DurationFiltering_2.count()) {
            FastestName = "Unbuffered Mutex";
        }
        else if (Fastest == DurationFiltering_3.count()){
            FastestName = "Unbuffered Atomic";
        }
        else if (Fastest == DurationFiltering_4.count()){
            FastestName = "Buffered Mutex";
        }
        else if (Fastest == DurationFiltering_5.count()){
            FastestName = "Buffered Atomic";
        }

        std::cout << "Fastest overall: " << FastestName << "\n\n";

    return 0;
}
