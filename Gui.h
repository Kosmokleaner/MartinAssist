#pragma once
#include "EveryHere.h"

class Gui
{
public:
    int test();
    
    std::string filter;

    EveryHere everyHere;

    void setViewDirty();
private:
    double whenToRebuildView = -1;
};

