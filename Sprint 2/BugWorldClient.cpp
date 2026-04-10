#include <iostream>
#include "BugWorldDisplayQt.h"
#include "BugWorldClientQt.h"
#include <string>
#include <sys/stat.h>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <QProcess>
#include <QObject>
#include <QWidget>
#include <QTimer>
#include <QFile>
#include <QSocketNotifier>
#include <QString>
#include <QCoreApplication>
#include <fcntl.h>
#include <unistd.h>
#include <filesystem>
#include <cstdio>
#include <QDir>

using namespace std;


FrameData parseFrame(const string& raw) {

    FrameData frame;
    istringstream in(raw);
    string line;
   
    while(getline(in, line)) {
        if(line.rfind("CYCLE", 0) == 0) {
            frame.Cycle = stoi(line.substr(6));
        }
        else if(line.rfind("MAP", 0) == 0) {
            sscanf(line.c_str(), "MAP %d %d", &frame.Dimensions[0], &frame.Dimensions[1]);
        }
        else if(line.rfind("ROW", 0) == 0) {
            frame.Map.push_back(line.substr(4));
        }
        else if(line.rfind("STATS", 0) == 0) {
            sscanf(line.c_str(), "STATS %d %d %d %d",
                &frame.stats[0], &frame.stats[1],
                &frame.stats[2], &frame.stats[3]);
        }
        else if(line.find("GAME OVER") != string::npos) {
            frame.Gameover = true;
        }
    }

    return frame;
}


BugWorldClientQt::BugWorldClientQt() {}

BugWorldClientQt::~BugWorldClientQt() {
    cleanup();
}


void BugWorldClientQt::start(string world, string red, string black){
 

    cmdPath = "/tmp/pipeline_cmd_" + QString::number(getpid());
    dataPath = "/tmp/pipeline_data_" + QString::number(getpid());

    string cmdPipe = cmdPath.toStdString();
    string dataPipe = dataPath.toStdString();

  
    unlink(cmdPipe.c_str());
    unlink(dataPipe.c_str());

    if (mkfifo(cmdPipe.c_str(), 0666) == -1) {
        perror("mkfifo cmd");
        throw runtime_error("Simulator failed to start");
    }
    if (mkfifo(dataPipe.c_str(), 0666) == -1) {
        perror("mkfifo data");
        unlink(cmdPipe.c_str());
        throw runtime_error("Simulator failed to start");
    }

    
    
    QStringList simArgs;
    simArgs << "--cmd-pipe" << cmdPath
            << "--data-pipe" << dataPath
            << QString::fromStdString(world) << QString::fromStdString(red)
                << QString::fromStdString(black);

  
    process.setWorkingDirectory(QDir::currentPath());
    process.start("../sim", simArgs);

    if(process.state() == QProcess::NotRunning){
        cout<<"process is not running, not launching the sim"<<endl;
    }

  
    if (!process.waitForStarted()) {
    cerr << "Error: Simulator failed to start." << process.errorString().toStdString()<<endl;
    cerr << process.readAllStandardError().toStdString() << endl;
    unlink(cmdPipe.c_str());
    unlink(dataPipe.c_str());
    throw runtime_error("simular failed to start");
    }
    cerr << "Sim PID: " << process.processId() << endl;


    connect(&process, &QProcess::readyReadStandardError, [this](){
    cerr << "SIM ERROR: " << process.readAllStandardError().toStdString() << endl;
});

    usleep(200000);
    

   
    cmd_fd = open(cmdPipe.c_str(), O_RDWR | O_NONBLOCK);

    if(cmd_fd == -1) throw runtime_error("cannot open command pipe , simulator cannot start");

    data_fd = open(dataPipe.c_str(), O_RDONLY | O_NONBLOCK);

    
    int flags = fcntl(data_fd, F_GETFL);
    fcntl(data_fd, F_SETFL, flags | O_NONBLOCK);

   
    poll_timer = new QTimer(this);
    connect(poll_timer, &QTimer::timeout, this, &BugWorldClientQt::read_data);
    poll_timer->start(10);

    cout << "Connected to Bug World Simulator." << endl;

}



void BugWorldClientQt::execute(int ticks, int fps){
    
    cout << "Connected to Bug World Simulator\n";

    this->ticks = ticks;
    this->timer_interval = 1000 /fps;
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &BugWorldClientQt::onTick);
    timer->start(timer_interval);
}
        

void BugWorldClientQt::cleanup(){
   
    if(cmd_fd != -1) {
        write(cmd_fd, "QUIT\n", 5);
        close(cmd_fd);
        cmd_fd = -1;
    }
    if(data_fd != -1) {
        close(data_fd);
        data_fd = -1;
    }

   
    if (process.state() != QProcess::NotRunning) {
        process.waitForFinished(3000);
    }

  
    unlink(cmdPath.toStdString().c_str());
    unlink(dataPath.toStdString().c_str());
}



void BugWorldClientQt::onTick(){
    
    string step = "STEP " + to_string(this->ticks) + "\n";

    ssize_t bytes_written = write(cmd_fd, step.c_str(), step.size());

    if(bytes_written == -1) {
    timer->stop();
    perror("write failed");
    return;
    }else if(bytes_written > 0){
        cout<<"everythings ok the client ignores us"<<endl;
    }

    timer->stop();

    cout<<"on tick checked"<<endl;

    /*
    QByteArray output;
    int timeoutCounter = 0;
    char buf[4096];
    bool done = false;

    while(!done){
        memset(buf, 0, sizeof(buf));
        ssize_t bytesRead = read(data_fd, buf, sizeof(buf) - 1);

        if(bytesRead > 0){
            buf[bytesRead] = '\0';
            output.append(buf);
            timeoutCounter = 0;
        if (output.contains("\nEND\n") || output.endsWith("END\n")) {
            done = true;
        }

        }
        // simulator closed early
        else if(bytesRead == 0){
        cerr << "Simulator closed the data pipe." << endl;
        timer->stop();
        //break;
        }

        // check if we are still receiving data
        else{
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                timeoutCounter++;
                usleep(50000);

                if (timeoutCounter>100){ // so 5 secs total of timeout
                    cerr <<"Not receiving anything anymore from Sim";
                    timer->stop();
                    //break;
                }
            }
            else {
                perror("read data_pipe");
                timer->stop();
                //break;
            }
        }
    }

    string outputPrinted = output.toStdString();
    FrameData Frame = parseFrame(outputPrinted);

    emit frameReady(Frame);

    // check if the game finished
    if(Frame.Gameover) {
    timer->stop();
    }

  */

}

void BugWorldClientQt::read_data() {

    cout<<"Entering read data"<<endl;

    char buffer[4096];
    

    ssize_t bytes_read = read(data_fd, buffer, sizeof(buffer) - 1);

  
    if (bytes_read == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return; 
        }
        perror("read error"); 
        return;
    }

    
    if (bytes_read == 0) {
        cerr << "Simulator closed the data pipe." << endl;
        if (timer) timer->stop();
        if (poll_timer) poll_timer->stop();
        return;
    }

   
    buffer[bytes_read] = '\0';
    incoming_buffer.append(buffer);

    
    if (incoming_buffer.contains("\nEND\n") || incoming_buffer.endsWith("END\n")) {
        string frame_text = incoming_buffer.toStdString();
        FrameData frame = parseFrame(frame_text);

        cout << "Frame " << frame.Cycle << " received. Emitting to GUI." << endl;

        emit frameReady(frame);
        incoming_buffer.clear(); 

        if (frame.Gameover) {
            cout << "Game Over detected." << endl;
            if (timer) timer->stop();
        } else {
            
            timer->start(100); 
        }
    }
}



//legacy code:
/*
void BugWorldClientQt::execute(int ticks, int fps){
    cout << "Connected to Bug World Simulator\n";

    // the main simulation loop
    bool running = true;
    while(true){

        // telling the simulator to move forward
        string step = "STEP " + to_string(ticks) + "\n";
        if(write(cmd_fd, step.c_str(), step.size()) == -1) break;

        QByteArray output;
        int timeoutCounter = 0;
        char buf[4096];
        bool done = false;

        // reading the data from the simulator until we see "END"
        while(!done){
            memset(buf, 0, sizeof(buf));
            ssize_t bytesRead = read(data_fd, buf, sizeof(buf) - 1);

            if(bytesRead > 0){
                buf[bytesRead] = '\0';
                output.append(buf);
                timeoutCounter = 0;
            if (output.contains("\nEND\n") || output.endsWith("END\n")) {
                done = true;
            }

            }
            // simulator closed early
            else if(bytesRead == 0){
            cerr << "Simulator closed the data pipe." << endl;
            running = false;
            break;
            }

            // check if we are still receiving data
            else{
                if(errno == EAGAIN || errno == EWOULDBLOCK){
                    timeoutCounter++;
                    usleep(50000);

                    if (timeoutCounter>100){ // so 5 secs total of timeout
                        cerr <<"Not receiving anything anymore from Sim";
                        running = false;
                        break;
                    }
                }
                else {
                    perror("read data_pipe");
                    running = false;
                    break;
                }
            }
        }

        string outputPrinted = output.toStdString();
        // show the frame on the screen
        display.clear();
        display.display_frame(outputPrinted);

        // check if the game finished
        if(outputPrinted.find("GAME OVER") != string::npos) {
            display.show_game_over_banner();
            // wait so the user can see the final state
            sleep(5);
            break;
        }
        // this controls the speed of the display
        usleep(1000000 / fps);
    }
}*/