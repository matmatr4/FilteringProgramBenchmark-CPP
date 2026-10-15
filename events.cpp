// Generates the synthetic detector events
// Every event has the same number of readings. Signal events (15%) contain
// enough strong readings to pass the filter; background events (85%) do not.
// 1% of events get a wrong checksum so the corruption check has work to do (only background events get corrupted)
#include "filtering-program.h"

#include <algorithm>
#include <random>

// method which adds up all readings of one event (used to create and later check against checksum)
// unit32_t is the variable type it gives out, the same as each variable inside 'checksum' array
// const means it will only read the variable parsed in as the argument, and whats after suggests the type
// followed by & means that the variable isn't copied, which improves performance and event means it will save whats parsed in under a variable called "event"
uint32_t sum_checksum(const std::vector<uint16_t>& event) {
    uint32_t sum = 0;

    // loop which iterated through every reading for the event passed in above saved as "event"
    for (uint16_t reading : event) {
        sum += reading;
    }
    return sum;
}

// method which generates events
// void means the function wont output anything thus only change the variables that were parsed in from the man file which calls this function
// in brackets the two arguments will be passed from the main function, under which the generated data will be saved
void generate_events(
    std::vector<std::vector<uint16_t>>& readings,
    std::vector<uint32_t>& checksum
) {
    readings.resize(NUM_EVENTS);
    checksum.resize(NUM_EVENTS);

    //initializing a random number generator called rng, 12345 inside brackets (seed) suggests it will produce the same sequence of numbers every run
    std::mt19937 rng(12345); // seed: the same events every run

    // nested loop for populating the nested array of events and checksum
    for (std::size_t e = 0; e < NUM_EVENTS; ++e) {
        readings[e].resize(EVENT_READINGS);

        // 1 identify if the reading is supposed to be valid or not

        // this variable will hold the number of strong readings 
        std::size_t strong_count;

        // 15 in 100 events are set to be valid, thus if the event number divided by 100 has a remainder of less than 15 it is set to have a number of strong reading which will allow it to be a kept event
        if (e % 100 < 15) {
            strong_count = SIGNAL_STRONG;      // signal event: 12,000 strong readings
        } else {
            strong_count = BACKGROUND_STRONG;  // background event: 2,000 strong readings
        }

        // 2 fill the readings list for the event

        // loop which traverses each reading of this event, and saves a generated value under it
        for (std::size_t i = 0; i < EVENT_READINGS; ++i) {
            // the first readings are always the 'strong' ones, which would be kept if compared against the threshold
            // if the current reading is bigger than the number of strong ones set earlier, this event is set to a noisy reading
            if (i < strong_count) {
                readings[e][i] = THRESHOLD + rng() % 3096; // adds threshold with a randomly generated number between 0-3096 (where the maximum possible is equal to 4096 due to the 12-bit readings)
            } else {
                readings[e][i] = rng() % (THRESHOLD - 100); // generates a number between 0-900 to ensure this number is below the threshold and can be discarded during filtering
            }
        }

        // 3 mix readings, to have random positions

        // loop to traverse every reading going from the back
        for (std::size_t i = EVENT_READINGS - 1; i > 0; --i) { //number of events is minused 1 as in an array with 6000 events, there will be 5999 indexes, as it starts from 0
            
            // initializing a number which will store a randomly generated value between 0 and i +1 since the last number is not included in generation so without +1 the max number would be 5998
            std::size_t j = rng() % (i + 1);

            // creating a temporary variable for a reading, which will store the current reading
            uint16_t temp = readings[e][i];
            // swapping the reading under i with the reading under j
            readings[e][i] = readings[e][j];
            readings[e][j] = temp;
        }

        // 4 checksum is made wrong on exactly 1 of every 100 events, of a background (discarded) event thus 15% kept event output is maintained
        
        // checksum list is populated with the sum_checksum function which produces a correct checksum for every event
        checksum[e] = sum_checksum(readings[e]);
        // for every event at position ..99 the checksum is corrupted, we are corrupting only invalid events, as the valid events are stored at positions ....0-..15
        if (e % 100 == 99) {
            checksum[e] = checksum[e] + 1;
        }

    }   
    
}
