#ifndef BUGWORLDDISPLAYQT_H
#define BUGWORLDDISPLAYQT_H

#include <string>
#include "FrameData.h"
#include <QObject>

using namespace std;

class BugWorldDisplayQt : public QObject {
    Q_OBJECT

    public:
        void clear();
        void display_frame(const string& output);
        void show_game_over_banner();
    public slots:
        void onFrameReady(FrameData data);
};


#endif 