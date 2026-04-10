#include <QWidget>
#include <QCloseEvent>
#include <QMessageBox>
#include <QStatusBar>
#include <QPushButton>
#include <QSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QKeyEvent>

#include "MainWindowQt.h"
#include "FrameData.h"
#include "MapCanvasQt.h"

MainWindow::MainWindow(int argc, char** argv, QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Bug World Game");
    resize(1000, 800);
    statusBar()->showMessage("System Ready");

    container = new QWidget(this);
    setCentralWidget(container);
    main_layout = new QHBoxLayout(container);

   
    client = new BugWorldClientQt();

   
    map_display = new MapCanvas();
    map_display->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    
    control_panel = new QWidget();
    control_layout = new QVBoxLayout(control_panel);
    control_panel->setFixedWidth(250); 

   
    start_button = new QPushButton("Start Simulation");
    tracking_steps = new QSpinBox();
    tracking_fps = new QSpinBox();
    world_input = new QLineEdit();
    red_input = new QLineEdit();
    black_input = new QLineEdit();

    
    tracking_fps->setRange(1, 60);
    tracking_fps->setValue(10);
    tracking_steps->setRange(0, 50);
    tracking_steps->setValue(5);

   
    world_input->setPlaceholderText("World file path...");
    red_input->setPlaceholderText("Red bug path...");
    black_input->setPlaceholderText("Black bug path...");

    
    control_layout->addWidget(new QLabel("<b>Simulation Controls</b>"));
    control_layout->addSpacing(10);
    control_layout->addWidget(new QLabel("FPS:"));
    control_layout->addWidget(tracking_fps);
    control_layout->addWidget(new QLabel("Trace Length (N):"));
    control_layout->addWidget(tracking_steps);
    control_layout->addSpacing(10);
    control_layout->addWidget(world_input);
    control_layout->addWidget(red_input);
    control_layout->addWidget(black_input);
    control_layout->addStretch(); 
    control_layout->addWidget(start_button);

    main_layout->addWidget(map_display, 4);  
    main_layout->addWidget(control_panel, 1); 

    
    connect(start_button, &QPushButton::clicked, this, &MainWindow::on_start_click);
    connect(client, &BugWorldClientQt::frameReady, this, &MainWindow::update_map);

    if (argc >= 4) {
        start_button->setEnabled(false);
        statusBar()->showMessage("Terminal Run Active - Sidebar Hidden (Press 'H' to toggle)");
        
        
        control_panel->hide();

        client->start(argv[1], argv[2], argv[3]);
        int ticks = (argc > 4) ? std::stoi(argv[4]) : 50;
        int fps = (argc > 5) ? std::stoi(argv[5]) : 20;
        client->execute(ticks, fps);
    }
}

void MainWindow::on_start_click() {
    QString world = world_input->text();
    QString red = red_input->text();
    QString black = black_input->text();

    if (world.isEmpty() || red.isEmpty() || black.isEmpty()) {
        statusBar()->showMessage("Error: All file paths are required.");
        return;
    }

    try {
        client->start(world.toStdString(), red.toStdString(), black.toStdString());
        client->execute(tracking_steps->value(), tracking_fps->value());
        start_button->setEnabled(false);
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Connection Error", e.what());
    }
}

void MainWindow::update_map(FrameData frame) {
    
    history.push_front(frame);

  
    int max_steps = tracking_steps->value();
    while (history.size() > (size_t)max_steps + 1) {
        history.pop_back();
    }

    QPoint pos = map_display->getBugCenter(frame, 'B', 20);
    cout << "Cycle: " << frame.Cycle << " | Blue Bug at: " << pos.x() << "," << pos.y() << std::endl;
    
    map_display->updateData(history, max_steps);
}


void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_H) {
        control_panel->setVisible(!control_panel->isVisible());
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (QMessageBox::question(this, "Exit", "Stop simulation and exit?", 
        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        event->accept();
    } else {
        event->ignore();
    }
}