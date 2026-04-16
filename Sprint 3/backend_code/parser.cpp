#include "world_state.h"   // World state data structure
#include <string>           // String class
#include <sstream>          // String streams for parsing

using namespace std;

// Parse simulator response string into a WorldState structure
// Input: raw - raw response text from simulator
// Returns: WorldState object with parsed data
WorldState parseResponse(const string& raw) {
    WorldState ws;                    // Create empty world state
    string line;
    istringstream stream(raw);         // Create input stream from raw string

    // Process response line by line
    while (getline(stream, line)) {
        // Skip empty lines
        if (line.empty()) continue;
        
        // Remove Windows line ending (\r) if present
        if (line.back() == '\r') line.pop_back();

        // Parse line by extracting keyword and values
        istringstream ss(line);
        string keyword;
        ss >> keyword;  // Read first token (command/keyword)

        // Handle CYCLE keyword - current simulation cycle number
        if (keyword == "CYCLE") {
            ss >> ws.cycle;  // Read cycle count

        // Handle MAP keyword - world dimensions
        } else if (keyword == "MAP") {
            ss >> ws.width >> ws.height;  // Read grid width and height

        // Handle ROW keyword - grid row data
        } else if (keyword == "ROW") {
            // Parse all cell characters in this row
            vector<char> row;
            char cell;
            while (ss >> cell)  // Read each cell character
                row.push_back(cell);
            ws.grid.push_back(row);  // Add row to grid

        // Handle STATS keyword - game statistics
        } else if (keyword == "STATS") {
            // Format: red_alive black_alive red_food black_food
            ss >> ws.red_alive >> ws.black_alive >> ws.red_food >> ws.black_food;
        }
        // END command and unrecognized keywords are skipped
    }

    // Return populated world state
    return ws;
}
