#ifndef FRAMEDATA_H
#define FRAMEDATA_H

#include <iostream>
#include <vector>
#include <string>

struct FrameData{
    int Cycle;
    int Dimensions[2];
    std::vector<std::string> Map;
    int stats[4];
    bool Gameover = false;
};

#endif