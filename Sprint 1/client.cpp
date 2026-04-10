#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <filesystem>
#include <string>
#include <sys/stat.h>
#include <fcntl.h>
#include <cerrno>
#include <algorithm>

using namespace std;

int main(int argc, char *argv[])
{

    // default values for fps and ticks
    int ticks_per_frame = 50;
    int fps = 10;

    if (argc < 4)
    {
        cerr << "Args needed for client: client <world_map> <bug1> <bug2>" << '\n';
        exit(1);
    }

    // arguments for client
    string world_map = argv[1];
    string bug1 = argv[2];
    string bug2 = argv[3];

    // setting ticks if given
    if (argc > 4)
    {
        ticks_per_frame = stoi(argv[4]);
    }

    // setting fps if given
    if (argc > 5)
    {
        fps = stoi(argv[5]);
    }

    // move the creation of the pipes and checking outside before the forking and outside the parent process
    // process scheduling may affect the logical sequence (the child process might launch the simulator before the pipes are created)

   
    const char *cmd_path = "./path_cmd";
    const char *data_path = "./path_data";
    mode_t mode = 0666;                         //read write permission set for the client and the simulator to both have permission to read write to the pipes


    //creating the named pipes in the directory
    int cmd = mkfifo(cmd_path, mode);
    int data = mkfifo(data_path, mode);

    if (cmd == -1 && errno != 17) // ignore pipe exists error
    {
        cerr << "Error creating the command pipe: " << errno << '\n';
        exit(1);
    }

    if (data == -1 && errno != 17) // ignore pipe exists error
    {
        cerr << "Error creating the data pipe: " << errno << '\n';
        exit(1);
    }


    //forking and launching the simulator
    //the child process will replace its image with the ./sim executables and passes the pipe path and file args to the simulator in order to boot

    pid_t pid;
    pid = fork();

    if (pid == 0)
    {
        // executing sim as child process
        int sim_status_code = execl("./sim", "./sim", "--cmd-pipe", "./path_cmd", "--data-pipe", "./path_data", world_map.c_str(), bug1.c_str(), bug2.c_str(), NULL);
        // added another ./sim in the execl function parameters, since the first argument is just the name of the program itself
        // changed the cpp strings world_map and bug1 and bug2 to C strings since the execl only understands C strings only( the function was receiving cpp string objects which the execl function may interpret wrongly)
        // changed the fifo paths inside execl to match the correct ones see line 46-47

        if (sim_status_code == -1)
        {
            cerr << "Sim pid did not terminate correctly: " << errno << '\n';
            exit(1);
        }
        
    }

    //the parent process pauses to give simulator time to initialize
    //connects the pipes, with the order mentioned in the README
    //parent process also contains the command loop and read loop 

    else if (pid > 0){

        usleep(100000);

        int cmd_pipe = open(cmd_path, O_WRONLY);
        int data_pipe = open(data_path, O_RDONLY | O_NONBLOCK); // if you add | O_NONBLOCK after compiling and running the first command you type will return nothing, only the second
                                                   // removing the | O_NONBLOCK maakes the first command typed in return something


        // logic steps - ask user for input, use getline() to capture users input, write the user input in cmd pipe, read using the data pipe and print result, the solution is to use a while loop that terminates when QUIT is entered

        // currently user inputs commands into terminal for testing purposes, CLI to be added
        while (1)
        {
            string user_input;
            cout << "Type a command: ";

            getline(cin, user_input);

            transform(user_input.begin(), user_input.end(), user_input.begin(), ::toupper); // lowercase input would also be accepted now

            if (user_input != "FETCH" && user_input != "QUIT" && user_input.find("STEP ") != 0) {
                cerr << "Invalid command: " << user_input << endl;
                continue; 
            }

            if (user_input == "QUIT")
            {
                user_input += "\n";
                write(cmd_pipe, user_input.c_str(), user_input.length());
                break;
            }

            user_input += "\n";

            write(cmd_pipe, user_input.c_str(), user_input.length());

            char read_buffer[100];
            ssize_t bytes_read;
            string response = "";

            while ((bytes_read = read(data_pipe, read_buffer, sizeof(read_buffer) - 1)) > 0)
            {
                read_buffer[bytes_read] = '\0';

                response += read_buffer;

                if (response.find("END") != string::npos)
                {
                    break;
                }
            }

            cout << response;
        }


        //ensure the parent process waits for the simulator to shutdown

        int status;
        waitpid(pid, &status, 0);

        if(WIFEXITED(status)){
            cout<<"Child exit status: "<<WEXITSTATUS(status)<<endl;
        }

        close(cmd_pipe);
        close(data_pipe);
    }
    else
    {
        cerr << "Fork failed: " << errno << endl;
        exit(1);
    }

    //unlinking the pipe files 

    unlink("./path_cmd");
    unlink("./path_data");
    return 0;
}