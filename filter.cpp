#include "filtering-program.h"

bool filter_event(
    std::vector<uint16_t>& event,
    uint32_t stored_checksum
) // both arguments are not read only
{
    //invalid events return false
    
    // discards events with corrupted checksum, through calculating each event checksum again and comparing it against the stored checksum, which is parsed from main function, where its generated alongside each event
    if (sum_checksum(event) != stored_checksum) {
        return false;
    }

    // check each reading against threshold and sum up the number of total valid readings under "strong"
    std::size_t strong = 0;

    // traverse each event's readings and count how many valid readings there are, and clear out the 
    for (std::size_t i = 0; i < event.size(); i = i + 1) {
        if (event[i] >= THRESHOLD) {
            event[strong] = event[i]; // saves the valid reading under the next valid position
            strong = strong + 1; //increments the number of valid readings
        }
    }
    event.resize(strong); // cuts the list to be the length of strong readings

    // checks if the event is valid based on the number of strong readings and the set minimum, if not it discards it
    if (strong < MIN_STRONG) {
        return false;
    }
    return true;
}