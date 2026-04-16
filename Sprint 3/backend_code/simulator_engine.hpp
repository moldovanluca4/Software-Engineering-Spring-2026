#pragma once          //ensures file is only included once during compilation to prevent duplicate definition errors
#include <string>

using namespace std;

// config struct to store all the stuff passed in from command line
//data container to hold all the settings needed to start the simulator
struct Config {
    string sim_binary = "./bin/sim";
    string world_file;
    string bug1_file;
    string bug2_file;
    int ticks_per_frame = 50;
    int fps = 10;
    bool auto_start = false;
};

class SimulatorEngine {
public:
    SimulatorEngine(const Config& config);
    ~SimulatorEngine();

    // dont want to deal with copying this, too many open fds
    //prevent copying this object since this class manages system resources copying it would cause two objects to try to control or close same processes or pipes leading to a crash
    SimulatorEngine(const SimulatorEngine&) = delete;
    SimulatorEngine& operator=(const SimulatorEngine&) = delete;

    void start();           // sets up pipes and launches sim
    string fetch();    // gets current state without stepping
    string step(int ticks);  // advance sim by n ticks
    void shutdown();        // cleanup everything
    

private:
    void writeCmd(const std::string& cmd);
    string readResponse();

    Config cfg;
    pid_t sim_pid = -1;
    int cmd_fd = -1;
    int data_fd = -1;
    string tmpdir;
    string cmd_pipe;
    string data_pipe;
};
