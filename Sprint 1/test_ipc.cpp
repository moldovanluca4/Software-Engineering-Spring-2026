#include <iostream>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fstream>
#include <cstring>

using namespace std;

int main() {
    //file system path to identify the named pipes
    const char* cmd_path = "./cmd_pipe";
    const char* data_path = "./data_pipe";

    //create fifos
    mkfifo(cmd_path, 0666);
    mkfifo(data_path, 0666);


    //parent process opens cmd pipe and writes the message, opens data pipe to read the reply, takes the reply and writes it inside the output file
    //child process opens cmd pipe and reads and waits for a message, receives the message and write the same message and sends it back, closes the connection and exits

    pid_t pid = fork();

    if (pid == 0) { 
        
        int read_cmd = open(cmd_path, O_RDONLY);
        int write_data = open(data_path, O_WRONLY);

        char buffer[100];
        ssize_t bytes = read(read_cmd, buffer, sizeof(buffer));

        if (bytes > 0){
        write(write_data, buffer, bytes);
        }
        
        close(read_cmd);
        close(write_data);
        return 0; 
    } 
    else if (pid > 0) {
        
        int write_cmd = open(cmd_path, O_WRONLY);
        int read_data = open(data_path, O_RDONLY);

        const char* data = "it works";
        write(write_cmd, data, strlen(data) + 1); 

        char buffer[100];
        ssize_t bytes = read(read_data, buffer, sizeof(buffer));
        
        if (bytes > 0) {
            ofstream output_file("output.txt");
            output_file << buffer;
            output_file.close();
        }

        close(write_cmd);
        close(read_data);
        wait(NULL); 
    }

    unlink(cmd_path);
    unlink(data_path);
    return 0;
}