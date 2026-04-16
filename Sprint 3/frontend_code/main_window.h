#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

// Qt framework includes for GUI elements
#include <QApplication>
#include <QMainWindow>      // Main application window class
#include <QWidget>          // Base class for all GUI elements
#include <QToolBar>         // Toolbar widget
#include <QVBoxLayout>      // Vertical layout manager
#include <QHBoxLayout>      // Horizontal layout manager
#include <QStatusBar>       // Status bar for messages
#include <QDockWidget>      // Dockable control panel
#include <QTextEdit>        // Text editing widget
#include <QLineEdit>        // Single-line text input
#include <iostream>         // Input/output operations
#include <QSpinBox>         // Number spinner input
#include <QSlider>          // Slider control
#include <QPushButton>      // Push button control
#include <deque>            // Double-ended queue for history


#include "simulator_engine.hpp"  // Simulator engine for running simulations
#include "world_state.h"         // Data structure for world state

using namespace std;

// Forward declaration of the parser function that converts raw simulator output to WorldState
WorldState parseResponse(const string& raw);

// Main GUI window class that displays the bug world simulation and controls
class main_window : public QMainWindow{
    Q_OBJECT

    private:
    // Layout management
    QHBoxLayout *main_layout;          // Main horizontal layout
    QVBoxLayout *control_layout;       // Vertical layout for control panel
    QWidget *container;                // Central widget container for drawing
    QDockWidget *dock_widget;          // Dockable control panel
    
    // Simulation engine
    SimulatorEngine *client_simulation; // Engine managing simulator process
    
    // File input fields
    QLineEdit *world_file_input;       // World file path input
    QLineEdit *red_input;              // Red team bug file path input
    QLineEdit *black_input;            // Black team bug file path input
    
    // Simulation parameters
    QSpinBox *ticks_input;             // Number of ticks per frame
    QSpinBox *fps_input;               // Frames per second
    
    // Control panel elements
    QWidget *control_panel;            // Container for all controls
    QTimer *timer;                     // Timer for updating animation frames
    QSlider *tracking_steps;           // Slider to control trace trail length
    
    // Buttons
    QPushButton *start_button;         // Start simulation button
    QPushButton *pause_button;         // Pause/resume simulation button
    QPushButton *browse_world_file;    // Browse world file button
    QPushButton *browse_red_file;      // Browse red bug file button
    QPushButton *browse_black_file;    // Browse black bug file button
    QPushButton *stop_button;
    
    // Layout
    QHBoxLayout *file_search_layout;   // File selection layout

    // State management
    bool is_paused = false;            // Pause state flag

    // Simulation data
    WorldState current_state;          // Current world state
    TraceState trace_state;            // Trace history for drawing position trails
    Config config;                     // Configuration settings

    // Event handlers
    void closeEvent(QCloseEvent *event) override;      // Handle window close event
    void paintEvent(QPaintEvent *event) override;      // Handle drawing/rendering
    void keyPressEvent(QKeyEvent * event) override;    // Handle keyboard input
    
    // History of world states for playback
    deque<WorldState> historical_path_of_the_bugs;     // Queue of past world states for animation
    
    public:
        // Constructor - initializes the main window with configuration
        explicit main_window(const Config& config, QWidget *parent = nullptr);

    private slots:  // Qt slot functions connected to button/slider signals
        void update_frame();                              // Update display with next simulation frame
        void update_trace_limit(int new_value);          // Update trace trail length from slider
        void start_simulation_after_start_button();      // Start/initialize simulation
        void pause_simulation_after_pause_button();      // Toggle pause/resume
        void browse_world_file_after_button_press();     
        void browse_red_file_after_button_press();       
        void browse_black_file_after_button_press();     
        void open_file_picker(QLineEdit* target_input, const QString& title, const QString& filter);
        void stop_simulation_clicked();
};

#endif