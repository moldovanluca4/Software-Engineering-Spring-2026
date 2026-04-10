#ifndef MAINWINDOWQT_H
#define MAINWINDOWQT_H

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QSpinBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <deque>

#include "BugWorldClientQt.h"
#include "FrameData.h"
#include "MapCanvasQt.h"

using namespace std;

class MainWindow : public QMainWindow {
    Q_OBJECT

    private:
        QHBoxLayout *main_layout;
        QVBoxLayout *control_layout;
        QPushButton *start_button;
        MapCanvas*map_display;
        QWidget *container;
        QWidget *dock_widget;
        QSpinBox *tracking_steps;
        BugWorldClientQt *client;
        QSpinBox *tracking_fps;
        QLineEdit *world_input;
        QLineEdit *red_input;
        QLineEdit *black_input;
        QWidget *control_panel;

        deque<FrameData> history;

        void closeEvent(QCloseEvent *event) override;
        void keyPressEvent(QKeyEvent *event) override;


    public:

    explicit MainWindow(int argc ,char** argv, QWidget *parent = nullptr);

    private slots:
        void on_start_click();
        void update_map(FrameData frame);
    
};






#endif