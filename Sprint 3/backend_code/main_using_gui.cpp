#include <iostream>
#include <QApplication>      // Qt application framework

#include "main_window.h"     // Main GUI window class

using namespace std;

int main(int argc, char* argv[]){
    // Initialize configuration with default values
    Config config;
    
    // Create Qt application object (manages GUI thread and event loop)
    QApplication app(argc, argv);

    // If command-line arguments provided, enable auto-start
    if(argc > 1){
        config.auto_start = true;
    }

    // Parse world file, red team program, and black team program from arguments
    if(argc > 3){ 
        config.world_file = argv[1];    // World map file
        config.bug1_file = argv[2];     // Red team bug program
        config.bug2_file = argv[3];     // Black team bug program
    }

    // Parse ticks per frame if provided
    if(argc > 4){
        config.ticks_per_frame = std::stoi(argv[4]);
    }

    // Parse FPS if provided
    if(argc > 5){
        config.fps = std::stoi(argv[5]);
    }
    
    // Set default simulation parameters
    config.sim_binary = "./bin/sim";            // Path to simulator binary
    config.world_file = "./worlds/tiny.world"; // Default world
    config.bug1_file = "./bugs/beetle.bug";   // Default red team
    config.bug2_file = "./bugs/beetle.bug";   // Default black team
    config.ticks_per_frame = 50;               // Simulate 50 ticks per frame
    config.fps = 10;                           // Target 10 frames per second

    // Validate parameters to ensure minimum values
    if (config.ticks_per_frame < 1) config.ticks_per_frame = 1;  // At least 1 tick
    if (config.fps < 1) config.fps = 1;                          // At least 1 fps

    // Create and display main window
    main_window window(config);
    window.show();

    // Start Qt event loop and execute application
    return app.exec();
}