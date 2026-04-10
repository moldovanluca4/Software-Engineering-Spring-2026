#include <QApplication>
#include "MainWindowQt.h"

using namespace std;

int main(int argc, char* argv[]){
    
    QApplication app(argc, argv);

    if(argc < 4){
        cerr<<"not enough arguments "<<" Usage: "<<argv[0] << " <world_map> <bug1> <bug2> [ticks] [fps]\n"<<endl;
        return 1;
    }

    
    MainWindow window(argc, argv);
    window.show();

   
    return app.exec();
}