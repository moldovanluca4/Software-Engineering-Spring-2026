#include "BugWorldDisplayQt.h"
#include <string>
#include <iostream>
#include <sstream>
#include "FrameData.h"

using namespace std;

const string RED_COLOR = "\033[1;31m";
const string BLACK_COLOR = "\033[1;34m";
const string FOOD_COLOR = "\033[1;32m";
const string RESET = "\033[0m";
const string BOLD = "\033[1m";
const string BORDER_COLOR = "\033[1;33m";

void BugWorldDisplayQt::clear(){
    cout << "\033[2J\033[1;1H";
}

void BugWorldDisplayQt::display_frame(const string& output){
    istringstream in(output);
        string line;
        int row = 0;

        while(getline(in, line)){
            if(line.rfind("CYCLE", 0) == 0){
                cout << BOLD << " [ CYCLE: " << line.substr(6) << " ]" << RESET << "\n\n";
            }
            else if(line.rfind("STATS", 0) == 0){
                cout << line << "\n";
            }
            else if(line.rfind("ROW", 0) == 0){
                // odd rows get a leading space
                if(row % 2 != 0) cout << " ";
                
                string row_chars = line.substr(4);
                for(char c : row_chars) {
                    if(c == 'R' || c == 'r') cout << RED_COLOR << "R" << RESET;
                    else if(c == 'B' || c == 'b') cout << BLACK_COLOR << "B" << RESET;
                    else if(isdigit(c)) cout << FOOD_COLOR << c << RESET;
                    else if (c == '#') cout << "\033[1;30m#\033[0m";
                    else cout << c;
                }
                cout << endl;
                row++;
            }
        }
}

void BugWorldDisplayQt::show_game_over_banner(){
        cout << "\n==========================================\n";
        cout << BORDER_COLOR << "       GAME OVER       " << RESET << "\n";
        cout << "\n==========================================\n";
}

void BugWorldDisplayQt::onFrameReady(FrameData Frame){
    BugWorldDisplayQt::clear();
    cout << "Cycle: "<< Frame.Cycle << endl;
    for(string line : Frame.Map){
        cout<< line<< endl;
    }
    cout << "Red alive: " << Frame.stats[0] << " Black alive: " << Frame.stats[1]
     << " Red food: " << Frame.stats[2] << " Black food: " << Frame.stats[3] << endl;
    if(Frame.Gameover){
        BugWorldDisplayQt::show_game_over_banner();
    }

}