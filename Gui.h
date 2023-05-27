#pragma once
#include "EveryHere.h"
#include "SelectionRange.h"

class Gui
{
public:
    Gui();

    int test();
    
    EveryHere everyHere;

    void setViewDirty();

private:
    //
    std::string filter;
    //
    double whenToRebuildView = -1;
    // 2: 1MB
    int32 minLogSize = 2;
    // into everyHere.driveView[]
    SelectionRange deviceSelectionRange;
    // into everyHere.fileView[]
    SelectionRangeWithDriveUpdate  fileSelectionRange;
    // number of frame until it triggers
    int triggerLoadOnStartup = 2; 

    ImGuiTableSortSpecs* fileSortCriteria = {};
};

