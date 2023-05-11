#pragma once
#include "EveryHere.h"

class Gui
{
public:
    int test();
    
    EveryHere everyHere;

    void setViewDirty();

private:
    //
    std::string filter;
    //
    double whenToRebuildView = -1;
    //
    int32 minLogSize = 0;
};

