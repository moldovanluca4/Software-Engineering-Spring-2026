#include <QPainter>
#include <QColor>
#include <QFontMetrics>
#include <QLabel>
#include <QMessageBox>
#include <QTimer>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QFileDialog>

#include "main_window.h"
#include "world_state.h"

main_window::main_window(const Config &cfg, QWidget *parent) : QMainWindow(parent), config(cfg) {
    // Set window title and initial size
    setWindowTitle("Bug World Simulation");
    resize(1200, 900);
    statusBar()->showMessage("Initializing simulator...");

    // Create dock widget for simulation controls (right side panel)
    dock_widget = new QDockWidget("Simulation Controls", this);
    control_panel = new QWidget(this);
    control_layout = new QVBoxLayout(control_panel);
    
    // Create input fields for file paths
    world_file_input = new QLineEdit();      // World map file path

    red_input = new QLineEdit();             // Red team bug program file

    black_input = new QLineEdit();           // Black team bug program file

    
    // Create slider for controlling trace trail length (0-50 steps)
    tracking_steps = new QSlider(Qt::Horizontal);
    tracking_steps->setRange(0, 50);
    tracking_steps->setValue(10);            // Default to 10 steps

    // Create spinboxes for simulation parameters
    ticks_input = new QSpinBox();            // Ticks to simulate per frame
    ticks_input->setRange(0,100);

    fps_input = new QSpinBox();              // Target frames per second
    fps_input->setRange(0,100);

    // Create control buttons
    start_button = new QPushButton("Start Simulation");

    pause_button = new QPushButton("Pause Simulation");

    stop_button = new QPushButton("Stop and Reset Simulation");

    browse_world_file = new QPushButton("Browse worlds");

    browse_red_file = new QPushButton("Browse bugs team 1");

    browse_black_file = new QPushButton("Browse bugs team 2");


    // Build control panel layout
    control_layout->addWidget(new QLabel("World File:"));
    QHBoxLayout* world_layout = new QHBoxLayout();
    world_layout->addWidget(world_file_input);
    world_layout->addWidget(browse_world_file);
    control_layout->addLayout(world_layout);


    control_layout->addWidget(new QLabel("Red Program:"));
    QHBoxLayout* red_layout = new QHBoxLayout();
    red_layout->addWidget(red_input);
    red_layout->addWidget(browse_red_file);
    control_layout->addLayout(red_layout);


    control_layout->addWidget(new QLabel("Black Program:"));
    QHBoxLayout* black_layout = new QHBoxLayout();
    black_layout->addWidget(black_input);
    black_layout->addWidget(browse_black_file);
    control_layout->addLayout(black_layout);


    control_layout->addWidget(new QLabel("Enter number of Ticks: "));
    control_layout->addWidget(ticks_input);

    control_layout->addWidget(new QLabel("Enter number of FPS: "));
    control_layout->addWidget(fps_input);

    control_layout->addWidget(new QLabel("Set steps: "));
    control_layout->addWidget(tracking_steps);

    control_layout->addWidget(new QLabel("Start Simulation"));
    control_layout->addWidget(start_button);

    control_layout->addWidget(new QLabel("Pause Simulation"));
    control_layout->addWidget(pause_button);
    pause_button->setEnabled(false);        // Disabled until simulation starts

    control_layout->addWidget(new QLabel("Reset State"));
    control_layout->addWidget(stop_button);
    stop_button->setEnabled(false);         // Disabled until simulation starts



    control_layout->addStretch();            // Fill remaining space
    
    // Load initial configuration values into input fields
    world_file_input->setText(QString::fromStdString(config.world_file));
    red_input->setText(QString::fromStdString(config.bug1_file));
    black_input->setText(QString::fromStdString(config.bug2_file));

    // Set up dock widget and add to main window
    control_panel->setLayout(control_layout);
    dock_widget->setWidget(control_panel);
    addDockWidget(Qt::RightDockWidgetArea, dock_widget);

    // Create central widget for drawing the simulation
    container = new QWidget(this);
    setCentralWidget(container);

    // Auto-start simulation if configured
    if(config.auto_start){
        QTimer::singleShot(0, this, &main_window::start_simulation_after_start_button);
    }

    // Connect signals to slots for UI interactions
    connect(tracking_steps, &QSlider::valueChanged, this, &main_window::update_trace_limit);
    connect(start_button, &QPushButton::released, this, &main_window::start_simulation_after_start_button);
    connect(pause_button, &QPushButton::released, this, &main_window::pause_simulation_after_pause_button);
    connect(browse_world_file, &QPushButton::released, this, &main_window::browse_world_file_after_button_press);
    connect(browse_red_file, &QPushButton::released, this, &main_window::browse_red_file_after_button_press);
    connect(browse_black_file, &QPushButton::released, this, &main_window::browse_black_file_after_button_press);
    connect(stop_button, &QPushButton::released, this, &main_window::stop_simulation_clicked);

}

// Start the simulation with user-provided configuration
void main_window::start_simulation_after_start_button(){
    // Get file paths from input fields
    string user_input_world_file = world_file_input->text().toStdString();
    string user_input_red_input = red_input->text().toStdString();
    string user_input_black_input = black_input->text().toStdString();

    // Update config with user inputs
    config.world_file = user_input_world_file;
    config.bug1_file = user_input_red_input;
    config.bug2_file = user_input_black_input;

    // Create and start the simulator engine
    client_simulation = new SimulatorEngine(config);
    try {
        client_simulation->start();      // Start the simulation process
        start_button->setEnabled(false); // Disable start button
        pause_button->setEnabled(true);  // Enable pause button
        stop_button->setEnabled(true);
        statusBar()->showMessage("Simulator Running");
    } catch (const exception& e) {
        // Show error dialog if startup fails
        QMessageBox::critical(this, "Error", e.what());
    }

    // Initialize trace history with default length
    trace_state.max_trace_len = 5;

    // Create timer to update display at configured FPS
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &main_window::update_frame);
    timer->start(1000 / config.fps);  // Start timer with FPS interval
}

// Toggle pause/resume state of the simulation
void main_window::pause_simulation_after_pause_button(){
    // If currently running, pause (stop timer)
    if(!is_paused){
        timer->stop();                           // Stop animation updates
        pause_button->setText("Resume");        // Change button text
        is_paused = true;                        // Set pause flag
        statusBar()->showMessage("Simulation Resumed");
    }else{
        // If paused, resume (restart timer)
        timer->start();                          // Restart animation updates
        pause_button->setText("Pause Simulation"); // Change button text
        is_paused = false;                       // Clear pause flag
        statusBar()->showMessage("Simulation Running");
    }
}

// Stop the simulation and reset the UI state
void main_window::stop_simulation_clicked() {
    
    if (timer && timer->isActive()) {
        timer->stop();
    }
    
    //Kill the backend process
    if (client_simulation) {
        client_simulation->shutdown();
        delete client_simulation;      
        client_simulation = nullptr;  
    }

    //clear the visual state
    historical_path_of_the_bugs.clear();
    trace_state.red_traces.clear();
    trace_state.black_traces.clear();
    update(); // Force a repaint to wipe the bugs off the screen

    //Reset the UI state
    start_button->setEnabled(true);
    pause_button->setEnabled(false);
    pause_button->setText("Pause Simulation");
    stop_button->setEnabled(false);
    is_paused = false;

    statusBar()->showMessage("Simulation Stopped. Ready for new configuration.");
}


// Browse for world files
void main_window::browse_world_file_after_button_press() {
    open_file_picker(world_file_input, "Select World File", "World Files (*.world);;All Files (*)");
}

void main_window::browse_red_file_after_button_press() {
    open_file_picker(red_input, "Select Red Team Bug", "Bug Files (*.bug);;All Files (*)");
}

void main_window::browse_black_file_after_button_press() {
    open_file_picker(black_input, "Select Black Team Bug", "Bug Files (*.bug);;All Files (*)");
}

void main_window::open_file_picker(QLineEdit* target_input, const QString& title, const QString& filter){
    QString file_path = QFileDialog::getOpenFileName(this, title, "./", filter);

    if (file_path.isEmpty()) {
        return; 
    }
    target_input->setText(file_path);
}


// Update the display frame - fetch next step from simulator and redraw
void main_window::update_frame() {
    try{
        // Request next simulation step from engine
        string raw_pos = client_simulation->step(config.ticks_per_frame);
        // Parse raw response into WorldState structure
        WorldState fresh_state = parseResponse(raw_pos);
        
        // Add new state to front of history deque
        historical_path_of_the_bugs.push_front(fresh_state);
        
        // Remove old states to keep history within max trace length
        while(historical_path_of_the_bugs.size() > (size_t)trace_state.max_trace_len + 1){
            historical_path_of_the_bugs.pop_back();
        }

        // Request repaint (triggers paintEvent)
        update();

    }catch(const exception& e){
        // Stop animation on error and display error message
        timer->stop();
        statusBar()->showMessage("Error: " + QString(e.what()));
    }
}

// Update the maximum length of the trace trail from slider input
void main_window::update_trace_limit(int new_value) {
    trace_state.max_trace_len = new_value;  // Update trace length limit

    // Remove excess history beyond new limit
    while(historical_path_of_the_bugs.size() > (size_t)trace_state.max_trace_len + 1){
        historical_path_of_the_bugs.pop_back();
    }

    // Redraw with new trace length
    update();
}
    




// Paint event handler - draws the simulation world and UI overlays
void main_window::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    // Create painter for drawing on the central widget
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);  // Smooth lines and circles

    // Exit early if no simulation data available
    if (historical_path_of_the_bugs.empty()) return;

    // Get current world state from front of history
    WorldState current_state = historical_path_of_the_bugs.front();

    // Get grid dimensions
    int rows = current_state.grid.size();
    int cols = current_state.grid[0].size();

    // Calculate cell size based on container dimensions
    float cell_w = (float)container->width() / (cols + 1);
    float cell_h = (float)container->height() / (rows + 1);
    float size = qMin(cell_w, cell_h);  // Use smaller dimension for square cells

    // Set drawing offsets
    float offset_x = 20;
    float offset_y = 40;

    // Draw grid cells
    for (int r = 0; r < rows; r++) {
        // Offset every other row for hex-like layout
        float row_indent = (r % 2 == 1) ? size / 2.0f : 0;
        for (int c = 0; c < cols; c++) {
            // Calculate cell position
            float x = offset_x + c * size + row_indent;
            float y = offset_y + r * size;
            QRectF rect(x, y, size * 0.9, size * 0.9);

            // Get cell character
            char cell = current_state.grid[r][c];
            auto pos = make_pair(r, c);

            // Draw background cell based on content
            painter.setPen(Qt::lightGray);
            painter.setBrush(Qt::NoBrush);
            if (cell == '#') {                          // Wall
                painter.setBrush(Qt::darkGray);
            } else if (cell >= '1' && cell <= '9') {   // Food
                painter.setBrush(Qt::yellow);
            }
            painter.drawEllipse(rect);

            // Draw red trace (fading trail of red bugs)
            if (trace_state.red_traces.count(pos)) {
                int age = trace_state.red_traces[pos];
                // Alpha decreases with age for fading effect
                int alpha = 255 * (1.0 - (float)age / (trace_state.max_trace_len + 1));
                painter.setBrush(QColor(255, 0, 0, alpha));
                painter.drawEllipse(rect);
            } else if (trace_state.black_traces.count(pos)) {  // Black trace
                int age = trace_state.black_traces[pos];
                int alpha = 255 * (1.0 - (float)age / (trace_state.max_trace_len + 1));
                painter.setBrush(QColor(0, 0, 0, alpha));
                painter.drawEllipse(rect);
            }

            // Draw bugs on top of traces
            if (cell == 'R' || cell == 'r') {          // Red bug (capital/lowercase variants)
                painter.setBrush(Qt::red);
                painter.drawEllipse(rect);
                painter.setPen(Qt::white);
                painter.drawText(rect, Qt::AlignCenter, "R");
            } else if (cell == 'B' || cell == 'b') {   // Black bug
                painter.setBrush(Qt::black);
                painter.drawEllipse(rect);
                painter.setPen(Qt::white);
                painter.drawText(rect, Qt::AlignCenter, "B");
            } else if (cell >= '1' && cell <= '9') {   // Food number
                painter.setPen(Qt::black);
                painter.drawText(rect, Qt::AlignCenter, QString(cell));
            }
        }
    }

    // Draw status overlay at top of window
    painter.setPen(Qt::black);
    painter.setFont(QFont("Monospace", 10, QFont::Bold));
    QString status = QString("Cycle: %1 | Trace N: %2 | Red: %3 (Food: %4) | Black: %5 (Food: %6)")
                         .arg(current_state.cycle)              // Current simulation cycle
                         .arg(trace_state.max_trace_len)        // Trace trail length
                         .arg(current_state.red_alive)          // Red bugs alive
                         .arg(current_state.red_food)           // Red food collected
                         .arg(current_state.black_alive)        // Black bugs alive
                         .arg(current_state.black_food);        // Black food collected
    painter.drawText(10, 20, status);
}

// Handle keyboard input
void main_window::keyPressEvent(QKeyEvent *event) {
    // 'H' key toggles visibility of control panel
    if (event->key() == Qt::Key_H) {
        dock_widget->setVisible(!dock_widget->isVisible());
        statusBar()->showMessage(QString("Trace length increased to %1").arg(trace_state.max_trace_len));
        event->accept();
    }else{
        // Pass other keys to parent class
        QMainWindow::keyPressEvent(event);
    }
}

// Handle window close event
void main_window::closeEvent(QCloseEvent *event) {
    // Ask user to confirm exit
    if (QMessageBox::question(this, "Exit", "Stop simulation and exit?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        event->accept();   // Allow window to close
    }else{
        event->ignore();   // Keep window open
    }
}