#include "simulator_engine.hpp"

#include <stdexcept>        // Standard exception handling
#include <string>           // String class
#include <cstring>          // C string functions
#include <cerrno>           // Error codes
#include <fcntl.h>          // File control operations (O_NONBLOCK, O_RDONLY, etc.)
#include <unistd.h>         // POSIX API (read, write, fork, execv, etc.)
#include <sys/stat.h>       // File status information
#include <sys/types.h>      // System data types
#include <sys/wait.h>       // Process wait operations
#include <signal.h>         // Signal handling
#include <time.h>           // Time functions
#include <fstream>          // File stream operations
#include <iostream>         // Input/output streams
#include <cerrno>           // Error codes
#include <poll.h>           // Poll system call for I/O multiplexing

using namespace std;

// Helper function to sleep for milliseconds
static void sleep_ms(long ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;                    // Seconds
    ts.tv_nsec = (ms % 1000) * 1000000L;     // Nanoseconds
    nanosleep(&ts, nullptr);                 // Sleep
}

// Constructor - stores configuration
SimulatorEngine::SimulatorEngine(const Config& config) : cfg(config) {}

// Destructor - cleanup resources safely
SimulatorEngine::~SimulatorEngine() {
    try {
        shutdown();  // Close pipes and terminate process
    } catch (...) {
        // Suppress exceptions in destructor
    }
}

// Start the simulator process and establish communication pipes
void SimulatorEngine::start() {
    // Create a temporary directory for named pipes
    char tmp[] = "/tmp/bugworld_XXXXXX";
    if (!mkdtemp(tmp))
        throw runtime_error("mkdtemp failed: " + string(strerror(errno)));
    tmpdir = tmp;  // Store temp directory path

    // Construct pipe file paths in temp directory
    cmd_pipe  = tmpdir + "/cmd.pipe";    // Commands to simulator
    data_pipe = tmpdir + "/data.pipe";   // Responses from simulator

    // Create named pipes (FIFOs) with restricted permissions (0600 = rw only by owner)
    if (mkfifo(cmd_pipe.c_str(), 0600) == -1)
        throw runtime_error("mkfifo cmd: " + string(strerror(errno)));
    if (mkfifo(data_pipe.c_str(), 0600) == -1)
        throw runtime_error("mkfifo data: " + string(strerror(errno)));

    // Fork a child process to run the simulator
    sim_pid = fork();
    if (sim_pid < 0)
        throw runtime_error("fork failed: " + string(strerror(errno)));

    if (sim_pid == 0) {
        // Child process: execute the simulator binary
        // Redirect stdout/stderr to log file for debugging
        int log_fd = open("sim_log.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (log_fd >= 0) {
            dup2(log_fd, 1);  // Redirect stdout to log file
            dup2(log_fd, 2);  // Redirect stderr to log file
            close(log_fd);
        }

        // Build argument list for simulator executable
        char* argv[] = {
            const_cast<char*>(cfg.sim_binary.c_str()),
            const_cast<char*>("--cmd-pipe"),
            const_cast<char*>(cmd_pipe.c_str()),
            const_cast<char*>("--data-pipe"),
            const_cast<char*>(data_pipe.c_str()),
            const_cast<char*>(cfg.world_file.c_str()),
            const_cast<char*>(cfg.bug1_file.c_str()),
            const_cast<char*>(cfg.bug2_file.c_str()),
            nullptr
        };
        // Replace child process with simulator
        execv(cfg.sim_binary.c_str(), argv);
        cerr << "execv has failed " << strerror(errno) << endl;
        _exit(1);  // Exit child if execv fails
    }

    // Parent process: Open pipes for communication
    data_fd = open(data_pipe.c_str(), O_RDONLY | O_NONBLOCK);
    if (data_fd < 0)
        throw runtime_error("open data pipe failed: " + string(strerror(errno)));

    int attempts = 0;
    while (attempts < 50) { // Try for 5 seconds total
        cmd_fd = open(cmd_pipe.c_str(), O_WRONLY | O_NONBLOCK);
        if (cmd_fd >= 0) {
            // Success! Switch command pipe to blocking mode
            int flags = fcntl(cmd_fd, F_GETFL);
            fcntl(cmd_fd, F_SETFL, flags & ~O_NONBLOCK);
            break;
        }

        if (errno != ENXIO) {
            throw runtime_error("open cmd pipe failed: " + string(strerror(errno)));
        }

        // Check if child process has crashed
        int status;
        if (waitpid(sim_pid, &status, WNOHANG) == sim_pid) { 
            throw runtime_error("simulator process died unexpectedly. check sim_log.txt");
        } else if (waitpid(sim_pid, &status, WNOHANG) == -1) {
            ofstream error_file("sim_log.txt");
            if (error_file.is_open()) {
                error_file << "waitpid failed with error: " << strerror(errno) << endl;
                error_file.close();
            }
            throw runtime_error("waitpid failed " + string(strerror(errno)));
        }

        usleep(100000); // Wait 100ms
        attempts++;
    }

    if (cmd_fd < 0) {
        throw runtime_error("timed out waiting for simulator to open cmd pipe");
    }
}

// Write a command to the simulator via the command pipe
void SimulatorEngine::writeCmd(const string& cmd) {
    if (cmd_fd < 0)
        throw runtime_error("engine not started yet");

    // Add newline to command
    string msg = cmd + "\n";
    const char* ptr = msg.c_str();
    ssize_t left = (ssize_t)msg.size();

    // Keep writing until entire message is sent
    while (left > 0) {
        ssize_t n = write(cmd_fd, ptr, left);
        if (n < 0) {
            // Interrupted system call - retry
            if (errno == EINTR) continue;
            throw runtime_error("write failed: " + string(strerror(errno)));
        }
        // Advance pointer and decrease bytes remaining
        ptr += n;
        left -= n;
    }
}


// Read simulator response from data pipe with timeout
string SimulatorEngine::readResponse() {
    string buf;                            // Accumulate response data
    char chunk[4096];                      // Read buffer
    struct pollfd pfd;
    pfd.fd = data_fd;
    pfd.events = POLLIN;                   // Wait for data available

    while (true) {
        // Wait for data with 2-second timeout
        int ret = poll(&pfd, 1, 2000);
        if (ret > 0) {
            // Data is available
            if (pfd.revents & POLLIN) {
                ssize_t n = read(data_fd, chunk, sizeof(chunk));
                if (n > 0) {
                    // Append new data to buffer
                    buf.append(chunk, n);

                    // Check if we've reached end of response
                    auto pos = buf.rfind("END");  // Find last occurence of "END"
                    if (pos != string::npos) {
                        // Verify "END" is on its own line
                        bool lineStart = (pos == 0 || buf[pos - 1] == '\n');
                        bool lineEnd = (pos + 3 == buf.size() || buf[pos + 3] == '\n' || buf[pos + 3] == '\r');
                        if (lineStart && lineEnd)
                            return buf;  // Complete response received
                    }
                } else if (n == 0) {
                    // EOF: simulator closed the pipe
                    throw runtime_error("sim closed the pipe unexpectedly");
                }
            }
            // Check for pipe errors
            if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                throw runtime_error("pipe error or hangup detected");
            }
        } else if (ret == 0) {
            // Timeout: no data received
            throw runtime_error("timeout waiting for simulator response");
        } else {
            // Poll error
            if (errno == EINTR) continue;  // Interrupted - retry
            throw runtime_error("poll failed: " + string(strerror(errno)));
        }
    }
}

// Fetch current state without advancing simulation
string SimulatorEngine::fetch() {
    writeCmd("FETCH");    // Send FETCH command
    return readResponse(); // Get response
}

// Step simulation forward by specified number of ticks
string SimulatorEngine::step(int ticks) {
    if (ticks < 1) ticks = 1;            // Ensure at least 1 tick
    writeCmd("STEP " + to_string(ticks)); // Send STEP command
    return readResponse();                 // Get new world state
}

// Shutdown simulator and cleanup all resources
void SimulatorEngine::shutdown() {
    // Try to gracefully shutdown simulator first
    if (cmd_fd >= 0) {
        try { writeCmd("QUIT"); } catch (...) {}  // Send quit command, ignore errors
        close(cmd_fd);
        cmd_fd = -1;
    }

    // Close data pipe
    if (data_fd >= 0) {
        close(data_fd);
        data_fd = -1;
    }

    // Wait for process to exit gracefully (up to 2 seconds)
    // If it doesn't exit, force kill it
    if (sim_pid > 0) {
        for (int i = 0; i < 20; i++) {  // 20 * 100ms = 2 seconds
            int status;
            if (waitpid(sim_pid, &status, WNOHANG) == sim_pid) {
                // Process exited
                sim_pid = -1;
                break;
            }
            sleep_ms(100);  // Wait 100ms before checking again
        }
        if (sim_pid > 0) {
            // Still running - force kill
            kill(sim_pid, SIGKILL);
            waitpid(sim_pid, nullptr, 0);
            sim_pid = -1;
        }
    }

    // Clean up temporary pipes and directory
    if (!cmd_pipe.empty())  { unlink(cmd_pipe.c_str());  cmd_pipe.clear(); }
    if (!data_pipe.empty()) { unlink(data_pipe.c_str()); data_pipe.clear(); }
    if (!tmpdir.empty())    { rmdir(tmpdir.c_str());      tmpdir.clear(); }
}
