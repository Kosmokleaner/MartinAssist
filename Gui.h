#pragma once
#include "EveryHere.h"
#include "SelectionRange.h"

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
    // into driveView[]
    SelectionRange deviceSelectionRange;
    // number of frame until it triggers
    int triggerLoadOnStartup = 2; 

    ImGuiTableSortSpecs* fileSortCriteria = {};
};

