#ifndef BUGWORLDCLIENTQT_H
#define BUGWORLDCLIENTQT_H

#include <iostream>
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
#include <QByteArray>

#include "FrameData.h"

using namespace std;



class BugWorldClientQt : public QObject {
    Q_OBJECT

private:
    QString cmdPath, dataPath;
    int cmd_fd ;
    int data_fd ;
    QProcess process;
    QTimer* timer = nullptr;
    int ticks;
    QSocketNotifier* data_notifier = nullptr;
    QByteArray incoming_buffer;
    QTimer *poll_timer;
    int timer_interval;

public:
    BugWorldClientQt();
    ~BugWorldClientQt();

    void start(string world, string red, string black);
    void execute(int ticks, int fps);
    void cleanup();
    

private slots:
    
    void onTick();
    void read_data();

signals:
    
    void frameReady(FrameData data);
};

#endif 