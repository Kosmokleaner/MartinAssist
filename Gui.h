#pragma once
#include "EveryHere.h"
#include "SelectionRange.h"

class Gui
{
public:
    Gui();

    int test();
    
    void guiDrives();
    void guiFiles();

    EveryHere everyHere;

    void setViewDirty();

private:
    //
    std::string filter;
    //
    double whenToRebuildView = -1;
    // 2: 1MB
    int32 minLogSize = 2;
    //
    int32 redundancyFilter = 0;
    // into everyHere.driveView[]
    SelectionRange driveSelectionRange;
    // into everyHere.fileView[]
    SelectionRangeWithDriveUpdate  fileSelectionRange;
    // number of frame until it triggers
    int triggerLoadOnStartup = 2; 

    ImGuiTableSortSpecs* fileSortCriteria = {};
};


void pushTableStyle3();

bool BeginTooltip();

void EndTooltip();


// @return printUnit e.g. "%.3f GB" or "%.3f MB" 
const char* computeReadableSize(uint64 inputSize, double& outPrintSize);


