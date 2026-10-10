#include "filtering-program.h"

int main() {

    // Two empty lists
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


    // function to generate events and fill the two lists above
    generate_events(readings, checksum);

}